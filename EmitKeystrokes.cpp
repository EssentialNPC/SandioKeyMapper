#include "stdafx.h"
#include <stdio.h>
#include <vector>
#include "EmitKeystrokes.h"

// SCondition: defines what circumstances must obtain in order for some action to be taken
struct SCondition {
	JoystickState::TWholeState relevant;	// these buttons are of interest (must be on or off)
	JoystickState::TWholeState required;	// these buttons must be on (others that are relevant must be off)
};

struct SKeyDefinition {
	WORD vk;
	WORD scan;
	DWORD flags;
};

struct SRuleState: public SCondition, public SKeyDefinition {
	bool on;

	void Clear() { memset(this, 0, sizeof(*this)); }
};

// A rule selection is a subset of rules in a sequence.
struct SRuleSelection {
	SRuleSelection(): nRules(0), ppRules(NULL) {}
	size_t       nRules;
	SRuleState **ppRules;
};

typedef std::vector<SRuleState> TRuleSequence;

TRuleSequence g_rules;
static const size_t k_nRuleSelections = 20;
SRuleSelection g_selections[k_nRuleSelections];		// one selection per button state bit: rules that may be affected by the flipping of that bit

void CalcRuleSelections(TRuleSequence &sequence, SRuleSelection *selections) {
	// clear old selections
	size_t iSel, iRule;
	for(iSel = 0; iSel < k_nRuleSelections; iSel++) {
		delete[] selections[iSel].ppRules;
	}
	memset(selections, sizeof(*selections)*k_nRuleSelections, 0);
	// count the number of rules per selection
	for(iRule = 0; iRule < sequence.size(); iRule++) {
		for(iSel = 0; iSel < k_nRuleSelections; iSel++) {
			if(sequence[iRule].relevant & (1 << iSel)) {
				selections[iSel].nRules++;
			}
		}
	}
	// allocate the selection arrays
	for(iSel = 0; iSel < k_nRuleSelections; iSel++) {
		if(selections[iSel].nRules > 0) {
			selections[iSel].ppRules = new SRuleState *[selections[iSel].nRules];
			selections[iSel].nRules = 0;
		}
	}
	// populate the selection arrays
	for(iRule = 0; iRule < sequence.size(); iRule++) {
		for(iSel = 0; iSel < k_nRuleSelections; iSel++) {
			if(sequence[iRule].relevant & (1 << iSel)) {
				selections[iSel].ppRules[selections[iSel].nRules] = &sequence[iRule];
				selections[iSel].nRules++;
			}
		}
	}
}

bool ParseConfig(const TCHAR *fileName, TCHAR *statusBuf, size_t nStatusChars) {
	// _sntprintf does not always terminate its strings, so let's hand-terminate
	nStatusChars--;
	statusBuf[nStatusChars] = 0;

	FILE *inFile = _tfopen(fileName, _T("rt"));
	if(!inFile) {
		_sntprintf(statusBuf, nStatusChars, "Failed to open config file: %s", fileName);
		return false;
	}
	const size_t nMaxBuf = 128;
	char buf[nMaxBuf];
	bool longLine = false;	// are we still reading a comment that exceeded nMaxBuf?
	int64_t nLines = 0;
	
	enum ESyllableDirections { SDForward, SDBack, SDLeft, SDRight, SDUp, SDDown, numSDirections };
	JoystickState::TWholeState s_mWordToBit[JoystickState::numButtons][numSDirections] = {
		{ 0x000002, 0x000008, 0x000004, 0x000001,        0,        0 },		// Top 3D Button:(Right=1 / Left=4 / Forward=2 / Backward=8)
		{ 0x000400, 0x000800, 0x000100, 0x000200, 0x000100, 0x000200 },		// Right 3D Button:(Forward=4 / Backward=8 / Up=1 / Down=2); Synonyms: Left is Up, Right is Down
		{ 0x010000, 0x040000, 0x020000, 0x080000, 0x080000, 0x020000 },		// Left 3D Button:(Forward=1 / Backward=4 / Up=8 / Down=2); Synonyms: Left is Down, Right is Up
	};
	struct SSyllableToDigit {
		SSyllableToDigit(const char *s, int d): syllable(s), digit(d), length(strlen(s)) {
			int z = 0;
		}

		const char *syllable;
		size_t      length;
		int         digit;
	};
	static const SSyllableToDigit s_SyllableToButton[] = {
		{ "Top", JoystickState::BTop },
		{ "T", JoystickState::BTop },
		{ "Right", JoystickState::BRight },
		{ "R", JoystickState::BRight },
		{ "Left", JoystickState::BLeft },
		{ "L", JoystickState::BLeft },
	};
	static const SSyllableToDigit s_SyllableToDir[] = {
		{ "Forwards", SDForward },
		{ "Forward", SDForward },
		{ "F", SDForward },

		{ "Backwards", SDBack },
		{ "Backward", SDBack },
		{ "Back", SDBack },
		{ "B", SDBack },

		{ "Left", SDLeft },
		{ "L", SDLeft },
		{ "Right", SDRight },
		{ "R", SDRight },
		{ "Up", SDUp },
		{ "U", SDUp },
		{ "Down", SDDown },
		{ "D", SDDown }
	};
	static const size_t k_nButtonSyllables = sizeof(s_SyllableToButton)/sizeof(s_SyllableToButton[0]);
	static const size_t k_nDirSyllables = sizeof(s_SyllableToDir)/sizeof(s_SyllableToDir[0]);
	static const char *s_buttonToSyllable[JoystickState::numButtons] = { "Top", "Right", "Left" };
	
	// key mapping
	struct SKeyNameAndValue {
		const char *name;
		SKeyDefinition def;
	};
	static SKeyNameAndValue s_keyNameToValue[] = {
		{"Space", {VK_SPACE, 0, 0}},
		{"Enter", {VK_RETURN, 0, 0}},
		{"Return", {VK_RETURN, 0, 0}},
		{"CR", {VK_RETURN, 0, 0}},
		{"Up", {VK_UP, 0, 0}},
		{"UpArrow", {VK_UP, 0, 0}},
		{"Down", {VK_DOWN, 0, 0}},
		{"DownArrow", {VK_DOWN, 0, 0}},
		{"DnArrow", {VK_DOWN, 0, 0}},
		{"Dn", {VK_DOWN, 0, 0}},
		{"Left", {VK_LEFT, 0, 0}},
		{"LeftArrow", {VK_LEFT, 0, 0}},
		{"Right", {VK_RIGHT, 0, 0}},
		{"RightArrow", {VK_RIGHT, 0, 0}},
		{"Home", {VK_HOME, 0, 0}},
		{"End", {VK_END, 0, 0}},
		{"PageUp", {VK_PRIOR, 0, 0}},
		{"PgUp", {VK_PRIOR, 0, 0}},
		{"PageDown", {VK_NEXT, 0, 0}},
		{"PgDown", {VK_NEXT, 0, 0}},
		{"PageDn", {VK_NEXT, 0, 0}},
		{"PgDn", {VK_NEXT, 0, 0}},
		{"Ins", {VK_INSERT, 0, 0}},
		{"Insert", {VK_INSERT, 0, 0}},
		{"Del", {VK_DELETE, 0, 0}},
		{"Delete", {VK_DELETE, 0, 0}},
		{"NumPad0", {VK_NUMPAD0, 0, 0}},
		{"NumPad1", {VK_NUMPAD1, 0, 0}},
		{"NumPad2", {VK_NUMPAD2, 0, 0}},
		{"NumPad3", {VK_NUMPAD3, 0, 0}},
		{"NumPad4", {VK_NUMPAD4, 0, 0}},
		{"NumPad5", {VK_NUMPAD5, 0, 0}},
		{"NumPad6", {VK_NUMPAD6, 0, 0}},
		{"NumPad7", {VK_NUMPAD7, 0, 0}},
		{"NumPad8", {VK_NUMPAD8, 0, 0}},
		{"NumPad9", {VK_NUMPAD9, 0, 0}},
		{"NumPad*", {VK_MULTIPLY, 0, 0}},
		{"NumPad/", {VK_DIVIDE, 0, 0}},
		{"NumPad+", {VK_ADD, 0, 0}},
		{"NumPad-", {VK_SUBTRACT, 0, 0}},
		{"Num0", {VK_NUMPAD0, 0, 0}},
		{"Num1", {VK_NUMPAD1, 0, 0}},
		{"Num2", {VK_NUMPAD2, 0, 0}},
		{"Num3", {VK_NUMPAD3, 0, 0}},
		{"Num4", {VK_NUMPAD4, 0, 0}},
		{"Num5", {VK_NUMPAD5, 0, 0}},
		{"Num6", {VK_NUMPAD6, 0, 0}},
		{"Num7", {VK_NUMPAD7, 0, 0}},
		{"Num8", {VK_NUMPAD8, 0, 0}},
		{"Num9", {VK_NUMPAD9, 0, 0}},
		{"Num*", {VK_MULTIPLY, 0, 0}},
		{"Num/", {VK_DIVIDE, 0, 0}},
		{"Num+", {VK_ADD, 0, 0}},
		{"Num-", {VK_SUBTRACT, 0, 0}},
		{"F1", {VK_F1, 0, 0}},
		{"F2", {VK_F2, 0, 0}},
		{"F3", {VK_F3, 0, 0}},
		{"F4", {VK_F4, 0, 0}},
		{"F5", {VK_F5, 0, 0}},
		{"F6", {VK_F6, 0, 0}},
		{"F7", {VK_F7, 0, 0}},
		{"F8", {VK_F8, 0, 0}},
		{"F9", {VK_F9, 0, 0}},
		{"F10", {VK_F10, 0, 0}},
		{"F11", {VK_F11, 0, 0}},
		{"F12", {VK_F12, 0, 0}},
		{"F13", {VK_F13, 0, 0}},
		{"F14", {VK_F14, 0, 0}},
		{"F15", {VK_F15, 0, 0}},
		{"F16", {VK_F16, 0, 0}},
		{"F17", {VK_F17, 0, 0}},
		{"F18", {VK_F18, 0, 0}},
		{"F19", {VK_F19, 0, 0}},
		{"F20", {VK_F20, 0, 0}},
		{"F21", {VK_F21, 0, 0}},
		{"F22", {VK_F22, 0, 0}},
		{"F23", {VK_F23, 0, 0}},
		{"F24", {VK_F24, 0, 0}},
	};
	static const size_t k_nKeyNames = sizeof(s_keyNameToValue)/sizeof(s_keyNameToValue[0]);
	
	try {
		while(fgets(buf, nMaxBuf, inFile)) {
			if(longLine) {
				if(*buf) {
					longLine = (buf[strlen(buf) - 1] != '\n');
				}
				continue;
			}

			nLines++;

			bool newWord = true;
			bool negative = false;
			int iButton = -1;
			int iDirection = -1;
			bool haveDirections = false;
			char *pos = buf;
			bool conditionDone = false;

			// Is this line a comment or a blank line?
			while(*pos == ' ' || *pos == '\t') {
				pos++;
			}
			if(*pos == '#' || *pos == '\n' || *pos == '\r') {
				continue;
			}

			SRuleState newRule;
			newRule.Clear();

			// Read the condition(s)
			while(*pos && !conditionDone) {
				if(iButton < 0) {
					for(size_t iSyl = 0; iSyl < k_nButtonSyllables; iSyl++) {
						if(strnicmp(pos, s_SyllableToButton[iSyl].syllable, s_SyllableToButton[iSyl].length) == 0) {
							iButton = s_SyllableToButton[iSyl].digit;
							haveDirections = false;
							pos += s_SyllableToButton[iSyl].length;
							break;
						}
					}
					if(iButton < 0) {
						switch(*pos) {
						case ':':
							conditionDone = true;
							// fall through to whitespace
						case ' ':
						case '\t':
							if(negative & !newWord) {
								_sntprintf(statusBuf, nStatusChars, "Line %lld at \"%s\": word ends without a button specification.",
										   nLines, buf);
								throw 1;
							}
							newWord = true;
							negative = false;
							break;
						case '!':
							negative = true;
							newWord = false;
							break;
						default:
							*pos = 0;
							_sntprintf(statusBuf, nStatusChars, "Line %lld at \"%s\": unexpected character.",
										nLines, buf);
							throw 2;
						}
						pos++;
					}
				} else {
					iDirection = -1;
					for(size_t iSyl = 0; iSyl < k_nDirSyllables; iSyl++) {
						if(strnicmp(pos, s_SyllableToDir[iSyl].syllable, s_SyllableToDir[iSyl].length) == 0) {
							iDirection = s_SyllableToDir[iSyl].digit;
							pos += s_SyllableToDir[iSyl].length;
							haveDirections = true;

							JoystickState::TWholeState bit = s_mWordToBit[iButton][iDirection];
							if(!bit) {
								*pos = 0;
								_sntprintf(statusBuf, nStatusChars, "Line %lld at \"%s\": no such direction: %s has no %s direction.",
											nLines, buf, s_buttonToSyllable[iButton], s_SyllableToDir[iSyl].syllable);
								throw 3;
							}
							if(newRule.relevant & bit) {
								*pos = 0;
								_sntprintf(statusBuf, nStatusChars, "Line %lld at \"%s\": duplicate condition:  this rule already requires %s %s to be %s.",
											nLines, buf, s_buttonToSyllable[iButton], s_SyllableToDir[iSyl].syllable, ((newRule.required & bit) ? "ON" : "OFF"));
								throw 4;
							}
							newRule.relevant |= bit;
							if(!negative) {
								newRule.required |= bit;
							}
							break;
						}
					}
					if(iDirection < 0) {
						switch(*pos) {
						case ':':
							conditionDone = true;
							// fall through to whitespace
						case ' ':
						case '\t':
							if(!haveDirections) {
								*pos = 0;
								_sntprintf(statusBuf, nStatusChars, "Line %lld at \"%s\": no direction specified for %s button.",
											nLines, buf, s_buttonToSyllable[iButton]);
								throw 5;
							}
							newWord = true;
							negative = false;
							iButton = -1;
							break;
						case '!':
							negative = true;
							break;
						}
						pos++;
					}
				}
			}
			if(!newRule.relevant) {
				*pos = 0;
				_sntprintf(statusBuf, nStatusChars, "Line %lld at \"%s\": empty condition list.",
							nLines, buf, s_buttonToSyllable[iButton]);
				throw 6;
			}

			// find beginning and end of the word describing the key
			while(*pos == ' ' || *pos == '\t') {
				pos++;
			}
			char *end = pos;
			while(' ' < *end && *end < 0x7F) {
				end++;
			}
			// skip the rest of the line (it's probably a comment) but make sure we find the newline
			if(*end) {
				longLine = (end[strlen(end) - 1] != '\n');
				*end = 0;
			} else {
				longLine = true;
			}
			// Interpret the key definition
			if(end == pos + 1) {	// simple character
				newRule.vk = toupper(*pos);
			} else if (sscanf(pos, "%hi,%hi,%i", &newRule.vk, &newRule.scan, &newRule.flags) >= 2) {
				// explicit specification, use it
			} else{	// a word that is hopefully in our table
				*end = 0;
				size_t iKeyName;
				for(iKeyName = 0; iKeyName < k_nKeyNames; iKeyName++) {
					if(stricmp(s_keyNameToValue[iKeyName].name, pos) == 0) {
						static_cast<SKeyDefinition &>(newRule) = s_keyNameToValue[iKeyName].def;
						break;
					}
				}
				if(iKeyName >= k_nKeyNames) {
					_sntprintf(statusBuf, nStatusChars, "Line %lld at \"%s\": unknown key name \"%s\".",
								nLines, buf, pos);
					throw 7;
				}
			}
			if(!newRule.scan) {
				newRule.scan = MapVirtualKey(newRule.vk, MAPVK_VK_TO_VSC);
			}

			g_rules.push_back(newRule);
		}
		if(g_rules.empty()) {
			_sntprintf(statusBuf, nStatusChars, "Line %lld: no rules found in file \"%s\".",
						nLines, fileName);
			throw 8;
		}
		fclose(inFile);
		CalcRuleSelections(g_rules, g_selections);
		_sntprintf(statusBuf, nStatusChars, "Loaded %ld rule(s) from %lld line(s) of text.",
					g_rules.size(), nLines);
		return true;
	} catch(int) {
		fclose(inFile);
		g_rules.clear();
		return false;
	}
}
#if 0
static SKeyDefinition g_runtimeMap[JoystickState::numButtons][JoystickState::dirsPerButton] = {
	{ {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, },
	{ {0, 0, 0}, {VK_SPACE, 0, 0}, {'2', 0, 0}, {VK_F13, 0, 0}, },
	{ {0x57, 0x11, 0}, {0x41, 0x1e, 0}, {0x53, 0x1f, 0}, {0x44, 0x20, 0}, },
};

void InitializeVirtualKeys() {
	for(size_t button = 0; button < JoystickState::numButtons; button++) {
		for(size_t dir = 0; dir < JoystickState::dirsPerButton; dir++) {
			if(g_runtimeMap[button][dir].vk && !g_runtimeMap[button][dir].scan) {
				g_runtimeMap[button][dir].scan = MapVirtualKey(g_runtimeMap[button][dir].vk, MAPVK_VK_TO_VSC);
			}
		}
	}
}

void AcceptMouseState(const JoystickState &newState)
{
	JoystickState changes = newState.wholesale ^ g_oldJoystickState.wholesale;
	if(changes) {
		INPUT input;
		input.type = INPUT_KEYBOARD;
		input.ki.time = 0;
		input.ki.dwExtraInfo = 0;
		for(size_t button = 0; button < JoystickState::numButtons; button++) {
			for(size_t dir = 0; dir < JoystickState::dirsPerButton; dir++) {
				uint8_t bit = 1 << dir;
				if(changes.buttons[button] & bit) {
					const SKeyDefinition &mapping = g_runtimeMap[button][dir];
					if(mapping.vk || mapping.scan) {
						input.ki.wVk = mapping.vk;
						input.ki.wScan = mapping.scan;
						input.ki.dwFlags = mapping.flags | ((newState.buttons[button] & bit) ? 0 : 0x2);
#if 0
						if(newState.buttons[button] & bit) {
							Sleep(1000);
						}
#endif
						SendInput(1, &input, sizeof(input));
					}
				}
			}
		}
		g_oldJoystickState = newState;
	}
}
#endif

inline bool ConsiderRule(INPUT &input, SRuleState &rule, JoystickState::TWholeState newState) {
	bool newStatus = (newState & rule.relevant) == rule.required;
	if(newStatus != rule.on) {
		input.ki.wVk = rule.vk;
		input.ki.wScan = rule.scan;
		input.ki.dwFlags = rule.flags;
		if(!newStatus) {
			input.ki.dwFlags |= 0x2;
		}
		rule.on = newStatus;
#if 0
		if(newState.buttons[button] & bit) {
			Sleep(1000);
		}
#endif
		SendInput(1, &input, sizeof(input));
		return true;
	} else {
		return false;
	}
}

static JoystickState g_oldJoystickState = {0};

#pragma intrinsic(_BitScanForward)

bool AcceptMouseState(const JoystickState &newState) {
	JoystickState changes = newState.wholesale ^ g_oldJoystickState.wholesale;
	// Most common case is no change.
	if(changes) {
		INPUT input;
		input.type = INPUT_KEYBOARD;
		input.ki.time = 0;
		input.ki.dwExtraInfo = 0;

		if(!(changes.wholesale & (changes.wholesale - 1))) {
			// Second most common case: one change.  Go through any rules that mention the bit that changed.
			DWORD pos = 0;
			_BitScanForward(&pos, changes.wholesale);
			if(pos < k_nRuleSelections) {
				SRuleSelection *selection = g_selections + pos;
				for(size_t iRule = 0; iRule < selection->nRules; iRule++) {
					ConsiderRule(input, *(selection->ppRules[iRule]), newState.wholesale);
				}
			}
		} else {
			// Several changes.  Go through all the rules.
			for(size_t iRule = 0; iRule < g_rules.size(); iRule++) {
				ConsiderRule(input, g_rules[iRule], newState.wholesale);
			}
		}
		g_oldJoystickState = newState;
		return true;
	}
	return false;
}
