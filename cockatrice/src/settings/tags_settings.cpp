#include "tags_settings.h"

TagsSettings::TagsSettings(const QString &settingPath, QObject *parent)
    : SettingsManager(settingPath + "tags.ini", parent)
{
}

QStringList TagsSettings::getDefaultTags()
{
    auto value = getValue("tags", "defaults");
    if (value.isNull()) {
        return DEFAULT_TAGS;
    }
    return value.toStringList();
}

void TagsSettings::setDefaultTags(const QStringList &tags)
{
    setValue(tags, "tags", "defaults");
    emit defaultTagsChanged();
}
