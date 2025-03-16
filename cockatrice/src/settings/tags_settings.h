#ifndef TAG_SETTINGS_H
#define TAG_SETTINGS_H

#include "settings_manager.h"

inline QStringList DEFAULT_TAGS = {
    // Strategies
    "🏃️ Aggro",
    "🧙‍️ Control",
    "⚔️ Midrange",
    "🌀 Combo",
    "🪓 Mill",
    "🔒 Stax",
    "🗺️ Landfall",
    "🛡️ Pillowfort",
    "🌱 Ramp",
    "⚡ Storm",
    "💀 Aristocrats",
    "☠️ Reanimator",
    "👹 Sacrifice",
    "🔥 Burn",
    "🌟 Lifegain",
    "🔮 Spellslinger",
    "👥 Tokens",
    "🎭 Blink",
    "⏳ Time Manipulation",
    "🌍 Domain",
    "💫 Proliferate",
    "📜 Saga",
    "🎲 Chaos",
    "🪄 Auras",
    "🔫 Pingers",

    // Themes
    "👑 Monarch",
    "🚀 Vehicles",
    "💉 Infect",
    "🩸 Madness",
    "🌀 Morph",

    // Card Types
    "⚔️ Creature",
    "💎 Artifact",
    "🌔 Enchantment",
    "📖 Sorcery",
    "⚡ Instant",
    "🌌 Planeswalker",
    "🌏 Land",
    "🪄 Aura",

    // Kindred Types
    "🐉 Kindred",
    "🧙 Humans",
    "⚔️ Soldiers",
    "🛡️ Knights",
    "🎻 Bards",
    "🧝 Elves",
    "🌲 Dryads",
    "😇 Angels",
    "🎩 Wizards",
    "🧛 Vampires",
    "🦴 Skeletons",
    "💀 Zombies",
    "👹 Demons",
    "👾 Eldrazi",
    "🐉 Dragons",
    "🐠 Merfolk",
    "🦁 Cats",
    "🐺 Wolves",
    "🐺 Werewolves",
    "🦇 Bats",
    "🐀 Rats",
    "🦅 Birds",
    "🦗 Insects",
    "🍄 Fungus",
    "🐚 Sea Creatures",
    "🐗 Boars",
    "🦊 Foxes",
    "🦄 Unicorns",
    "🐘 Elephants",
    "🐻 Bears",
    "🦏 Rhinos",
    "🦂 Scorpions",
};

class TagsSettings : public SettingsManager
{
    Q_OBJECT
    friend class SettingsCache;

    explicit TagsSettings(const QString &settingPath, QObject *parent = nullptr);
    TagsSettings(const TagsSettings & /*other*/);

public:
    QStringList getDefaultTags();

    void setDefaultTags(const QStringList &tags);

signals:
    void defaultTagsChanged();
};

#endif // TAG_SETTINGS_H
