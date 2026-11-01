// elevate.h -- 提权 / UIAccess / 热键
#pragma once

bool IsElevated();                                   // [0x1400014a0]

// 成功时返回 true。若通过"重启自身"达成，*relaunched = true，调用方应退出。
bool TryGetUIAccess(bool* relaunched);               // [0x140034f60]

void StartHotkeyThread();                            // [0x140034de0]
