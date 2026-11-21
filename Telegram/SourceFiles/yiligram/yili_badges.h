/*
This file is part of YiliGram Desktop,
an unofficial Telegram client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/priyanikkk/YiliGram-Desktop/blob/main/LEGAL
*/
#pragma once

class PeerData;

namespace Ui {
class RpWidget;
} // namespace Ui

namespace Window {
class SessionController;
} // namespace Window

namespace Yili {

// YiliGram badges, shared with YiliGram for Android: the team, beta
// testers, official YiliGram channels and groups. The list is a message
// tagged #yiligram_badges in Saved Messages:
// {"admin":[ids],"beta":[ids],"official":[ids]}, users have positive ids,
// chats and channels negative ones.
enum class BadgeType : uchar {
	Admin,
	Beta,
	Official,
};

[[nodiscard]] std::optional<BadgeType> BadgeFor(not_null<PeerData*> peer);
[[nodiscard]] rpl::producer<> BadgesChanges();

void PaintBadge(QPainter &p, BadgeType type, const QRectF &rect);

void ShowBadgeInfo(
	not_null<Window::SessionController*> controller,
	BadgeType type);
void ShowBadgeGrant(
	not_null<Window::SessionController*> controller,
	not_null<PeerData*> peer);

// A clickable badge next to a name, hidden while the peer has no badge.
[[nodiscard]] not_null<Ui::RpWidget*> CreateBadgeButton(
	not_null<QWidget*> parent,
	not_null<Window::SessionController*> controller,
	not_null<PeerData*> peer,
	int size);

} // namespace Yili
