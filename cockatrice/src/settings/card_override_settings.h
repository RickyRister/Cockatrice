#ifndef COCKATRICE_CARD_OVERRIDE_SETTINGS_H
#define COCKATRICE_CARD_OVERRIDE_SETTINGS_H

#include "settings_manager.h"

#include <QObject>

class CardOverrideSettings : public SettingsManager
{
    Q_OBJECT
    friend class SettingsCache;

public:
    void setCardPreferenceOverride(const QString &cardName, const QString &providerId);

    void deleteCardPreferenceOverride(const QString &cardName);

    QString getCardPreferenceOverride(const QString &cardName);

private:
    explicit CardOverrideSettings(QString settingPath, QObject *parent = nullptr);
    CardOverrideSettings(const CardOverrideSettings & /*other*/);
};

#endif // COCKATRICE_CARD_OVERRIDE_SETTINGS_H