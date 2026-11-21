/*
This file is part of YiliGram Desktop,
an unofficial Telegram client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/priyanikkk/YiliGram-Desktop/blob/main/LEGAL
*/
#pragma once

#include "yiligram/yili_shapes.h"

namespace Yili {

// Reads tdata/yiligram.json, call once local storage is started.
void LoadSettings();

[[nodiscard]] bool BubbleTails();
void SetBubbleTails(bool enabled);

[[nodiscard]] bool HeaderCallButton();
void SetHeaderCallButton(bool enabled);

[[nodiscard]] bool HideGiftButton();
void SetHideGiftButton(bool hide);

void SaveAvatarFrame(AvatarFrame frame);

// Fired after any YiliGram setting changed.
[[nodiscard]] rpl::producer<> SettingsChanges();

// Raw access for the badges list, stored in the same file.
[[nodiscard]] QByteArray BadgesJson();
void SetBadgesJson(const QByteArray &json);

// Our own strings: not in Telegram's language packs.
[[nodiscard]] QString Tr(const char *en, const char *ru, const char *uk);

} // namespace Yili
