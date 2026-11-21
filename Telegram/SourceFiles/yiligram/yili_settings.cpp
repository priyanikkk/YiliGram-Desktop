/*
This file is part of YiliGram Desktop,
an unofficial Telegram client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/priyanikkk/YiliGram-Desktop/blob/main/LEGAL
*/
#include "yiligram/yili_settings.h"

#include "lang/lang_keys.h"
#include "settings.h"

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

namespace Yili {
namespace {

struct Values {
	bool bubbleTails = true;
	bool headerCallButton = true;
	bool hideGiftButton = false;
	int avatarFrame = 0;
	QByteArray badges;
};

Values Current;
rpl::event_stream<> Changes;

[[nodiscard]] QString Path() {
	return cWorkingDir() + u"tdata/yiligram.json"_q;
}

void Save() {
	auto object = QJsonObject();
	object.insert(u"bubbleTails"_q, Current.bubbleTails);
	object.insert(u"headerCallButton"_q, Current.headerCallButton);
	object.insert(u"hideGiftButton"_q, Current.hideGiftButton);
	object.insert(u"avatarFrame"_q, Current.avatarFrame);
	if (!Current.badges.isEmpty()) {
		const auto badges = QJsonDocument::fromJson(Current.badges);
		if (badges.isObject()) {
			object.insert(u"badges"_q, badges.object());
		}
	}
	auto file = QFile(Path());
	if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
	}
}

void Changed() {
	Save();
	for (const auto widget : QApplication::topLevelWidgets()) {
		widget->update();
	}
	Changes.fire({});
}

} // namespace

void LoadSettings() {
	auto file = QFile(Path());
	if (file.open(QIODevice::ReadOnly)) {
		const auto object = QJsonDocument::fromJson(file.readAll()).object();
		Current.bubbleTails = object.value(u"bubbleTails"_q).toBool(true);
		Current.headerCallButton = object.value(
			u"headerCallButton"_q).toBool(true);
		Current.hideGiftButton = object.value(
			u"hideGiftButton"_q).toBool(false);
		Current.avatarFrame = std::clamp(
			object.value(u"avatarFrame"_q).toInt(0),
			0,
			kAvatarFrameCount - 1);
		const auto badges = object.value(u"badges"_q);
		if (badges.isObject()) {
			Current.badges = QJsonDocument(badges.toObject()).toJson(
				QJsonDocument::Compact);
		}
	}
	SetAvatarFrame(AvatarFrame(Current.avatarFrame));
}

bool BubbleTails() {
	return Current.bubbleTails;
}

void SetBubbleTails(bool enabled) {
	if (Current.bubbleTails != enabled) {
		Current.bubbleTails = enabled;
		Changed();
	}
}

bool HeaderCallButton() {
	return Current.headerCallButton;
}

void SetHeaderCallButton(bool enabled) {
	if (Current.headerCallButton != enabled) {
		Current.headerCallButton = enabled;
		Changed();
	}
}

bool HideGiftButton() {
	return Current.hideGiftButton;
}

void SetHideGiftButton(bool hide) {
	if (Current.hideGiftButton != hide) {
		Current.hideGiftButton = hide;
		Changed();
	}
}

void SaveAvatarFrame(AvatarFrame frame) {
	if (Current.avatarFrame != int(frame)) {
		Current.avatarFrame = int(frame);
		SetAvatarFrame(frame);
		Changed();
	}
}

rpl::producer<> SettingsChanges() {
	return Changes.events();
}

QByteArray BadgesJson() {
	return Current.badges;
}

void SetBadgesJson(const QByteArray &json) {
	if (Current.badges != json) {
		Current.badges = json;
		Save();
	}
}

QString Tr(const char *en, const char *ru, const char *uk) {
	const auto id = Lang::Id();
	if (id.startsWith(u"ru"_q)) {
		return QString::fromUtf8(ru);
	} else if (id.startsWith(u"uk"_q)) {
		return QString::fromUtf8(uk);
	}
	return QString::fromUtf8(en);
}

} // namespace Yili
