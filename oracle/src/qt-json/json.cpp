/* Copyright 2011 Eeli Reilin. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ''AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL EELI REILIN OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of Eeli Reilin.
 */

/**
 * \file json.cpp
 */

#include "json.h"

#include <QMetaType>
#include <iostream>

namespace QtJson
{

static QString sanitizeString(QString str)
{
    str.replace(QLatin1String("\\"), QLatin1String("\\\\"));
    str.replace(QLatin1String("\""), QLatin1String("\\\""));
    str.replace(QLatin1String("\b"), QLatin1String("\\b"));
    str.replace(QLatin1String("\f"), QLatin1String("\\f"));
    str.replace(QLatin1String("\n"), QLatin1String("\\n"));
    str.replace(QLatin1String("\r"), QLatin1String("\\r"));
    str.replace(QLatin1String("\t"), QLatin1String("\\t"));
    return QString(QLatin1String("\"%1\"")).arg(str);
}

static QByteArray join(const QList<QByteArray> &list, const QByteArray &sep)
{
    QByteArray res;
    for (const QByteArray &i : list) {
        if (!res.isEmpty()) {
            res += sep;
        }
        res += i;
    }
    return res;
}

/**
 * parse
 */
QVariant Json::parse(const QString &json)
{
    bool success = true;
    return Json::parse(json, success);
}

/**
 * parse
 */
QVariant Json::parse(const QString &json, bool &success)
{
    success = true;

    // Return an empty QVariant if the JSON data is either null or empty
    if (!json.isNull() || !json.isEmpty()) {
        // We'll start from index 0
        int index = 0;

        // Parse the first value
        QVariant value = Json::parseValue(json, index, success);

        // Return the parsed value
        return value;
    } else {
        // Return the empty QVariant
        return QVariant();
    }
}

QByteArray Json::serialize(const QVariant &data)
{
    bool success = true;
    return Json::serialize(data, success);
}

QByteArray Json::serialize(const QVariant &data, bool &success)
{
    QByteArray str;
    success = true;

    if (!data.isValid()) // invalid or null?
    {
        str = "null";
    }
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    else if ((data.typeId() == QMetaType::Type::QVariantList) ||
             (data.typeId() == QMetaType::Type::QStringList)) // variant is a list?
#else
    else if ((data.type() == QVariant::List) || (data.type() == QVariant::StringList))  // variant is a list?
#endif
    {
        QList<QByteArray> values;
        const QVariantList list = data.toList();
        for (const QVariant &v : list) {
            QByteArray serializedValue = serialize(v);
            if (serializedValue.isNull()) {
                success = false;
                break;
            }
            values << serializedValue;
        }

        str = "[ " + join(values, ", ") + " ]";
    }
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    else if ((data.typeId() == QMetaType::Type::QVariantHash)) // variant is a list?
#else
    else if (data.type() == QVariant::Hash)                                             // variant is a hash?
#endif
    {
        const QVariantHash vhash = data.toHash();
        QHashIterator<QString, QVariant> it(vhash);
        str = "{ ";
        QList<QByteArray> pairs;

        while (it.hasNext()) {
            it.next();
            QByteArray serializedValue = serialize(it.value());

            if (serializedValue.isNull()) {
                success = false;
                break;
            }

            pairs << sanitizeString(it.key()).toUtf8() + " : " + serializedValue;
        }

        str += join(pairs, ", ");
        str += " }";
    }
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    else if ((data.typeId() == QMetaType::Type::QVariantMap)) // variant is a list?
#else
    else if (data.type() == QVariant::Map)                                              // variant is a map?
#endif
    {
        const QVariantMap vmap = data.toMap();
        QMapIterator<QString, QVariant> it(vmap);
        str = "{ ";
        QList<QByteArray> pairs;
        while (it.hasNext()) {
            it.next();
            QByteArray serializedValue = serialize(it.value());
            if (serializedValue.isNull()) {
                success = false;
                break;
            }
            pairs << sanitizeString(it.key()).toUtf8() + " : " + serializedValue;
        }
        str += join(pairs, ", ");
        str += " }";
    }
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    else if ((data.typeId() == QMetaType::Type::QString) ||
             (data.typeId() == QMetaType::Type::QByteArray)) // variant is a list?
#else
    else if ((data.type() == QVariant::String) || (data.type() == QVariant::ByteArray)) // a string or a byte array?
#endif
    {
        str = sanitizeString(data.toString()).toUtf8();
    }
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    else if (data.typeId() == QMetaType::Type::Double)
#else
    else if (data.type() == QVariant::Double)                                           // double?
#endif
    {
        str = QByteArray::number(data.toDouble(), 'g', 20);
        if (!str.contains(".") && !str.contains("e")) {
            str += ".0";
        }
    }
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    else if (data.typeId() == QMetaType::Type::Bool)
#else
    else if (data.type() == QVariant::Bool)                                             // boolean value?
#endif
    {
        str = data.toBool() ? "true" : "false";
    }
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    else if (data.typeId() == QMetaType::Type::ULongLong)
#else
    else if (data.type() == QVariant::ULongLong)                                        // large unsigned number?
#endif
    {
        str = QByteArray::number(data.value<qulonglong>());
    } else if (data.canConvert<qlonglong>()) // any signed number?
    {
        str = QByteArray::number(data.value<qlonglong>());
    } else if (data.canConvert<long>()) {
        str = QString::number(data.value<long>()).toUtf8();
    } else if (data.canConvert<QString>()) // can value be converted to string?
    {
        // this will catch QDate, QDateTime, QUrl, ...
        str = sanitizeString(data.toString()).toUtf8();
    } else {
        success = false;
    }
    if (success) {
        return str;
    } else {
        return QByteArray();
    }
}

/**
 * parseValue
 */
QVariant Json::parseValue(const QString &json, int &index, bool &success)
{
    // Determine what kind of data we should parse by
    // checking out the upcoming token
    switch (Json::lookAhead(json, index)) {
        case JsonTokenString:
            return Json::parseString(json, index, success);
        case JsonTokenNumber:
            return Json::parseNumber(json, index);
        case JsonTokenCurlyOpen:
            return Json::parseObject(json, index, success);
        case JsonTokenSquaredOpen:
            return Json::parseArray(json, index, success);
        case JsonTokenTrue:
            Json::nextToken(json, index);
            return QVariant(true);
        case JsonTokenFalse:
            Json::nextToken(json, index);
            return QVariant(false);
        case JsonTokenNull:
            Json::nextToken(json, index);
            return QVariant();
        case JsonTokenNone:
            break;
    }

    // If there were no tokens, flag the failure and return an empty QVariant
    success = false;
    return QVariant();
}

/**
 * parseObject
 */
QVariant Json::parseObject(const QString &json, int &index, bool &success)
{
    QVariantMap map;
    int token;

    // Get rid of the whitespace and increment index
    Json::nextToken(json, index);

    // Loop through all of the key/value pairs of the object
    bool done = false;
    while (!done) {
        // Get the upcoming token
        token = Json::lookAhead(json, index);

        if (token == JsonTokenNone) {
            success = false;
            return QVariantMap();
        } else if (token == JsonTokenComma) {
            Json::nextToken(json, index);
        } else if (token == JsonTokenCurlyClose) {
            Json::nextToken(json, index);
            return map;
        } else {
            // Parse the key/value pair's name
            QString name = Json::parseString(json, index, success).toString();

            if (!success) {
                return QVariantMap();
            }

            // Get the next token
            token = Json::nextToken(json, index);

            // If the next token is not a colon, flag the failure
            // return an empty QVariant
            if (token != JsonTokenColon) {
                success = false;
                return QVariant(QVariantMap());
            }

            // Parse the key/value pair's value
            QVariant value = Json::parseValue(json, index, success);

            if (!success) {
                return QVariantMap();
            }

            // Assign the value to the key in the map
            map[name] = value;
        }
    }

    // Return the map successfully
    return QVariant(map);
}

/**
 * parseArray
 */
QVariant Json::parseArray(const QString &json, int &index, bool &success)
{
    QVariantList list;

    Json::nextToken(json, index);

    bool done = false;
    while (!done) {
        int token = Json::lookAhead(json, index);

        if (token == JsonTokenNone) {
            success = false;
            return QVariantList();
        } else if (token == JsonTokenComma) {
            Json::nextToken(json, index);
        } else if (token == JsonTokenSquaredClose) {
            Json::nextToken(json, index);
            break;
        } else {
            QVariant value = Json::parseValue(json, index, success);

            if (!success) {
                return QVariantList();
            }

            list.push_back(value);
        }
    }

    return QVariant(list);
}

/**
 * parseString
 */
QVariant Json::parseString(const QString &json, int &index, bool &success)
{
    QString s;
    QChar c;

    Json::eatWhitespace(json, index);

    c = json[index++];

    bool complete = false;
    while (!complete) {
        if (index == json.size()) {
            break;
        }

        c = json[index++];

        if (c == '\"') {
            complete = true;
            break;
        } else if (c == '\\') {
            if (index == json.size()) {
                break;
            }

            c = json[index++];

            if (c == '\"') {
                s.append('\"');
            } else if (c == '\\') {
                s.append('\\');
            } else if (c == '/') {
                s.append('/');
            } else if (c == 'b') {
                s.append('\b');
            } else if (c == 'f') {
                s.append('\f');
            } else if (c == 'n') {
                s.append('\n');
            } else if (c == 'r') {
                s.append('\r');
            } else if (c == 't') {
                s.append('\t');
            } else if (c == 'u') {
                int remainingLength = json.size() - index;

                if (remainingLength >= 4) {
                    QString unicodeStr = json.mid(index, 4);

                    int symbol = unicodeStr.toInt(0, 16);

                    s.append(QChar(symbol));

                    index += 4;
                } else {
                    break;
                }
            }
        } else {
            s.append(c);
        }
    }

    if (!complete) {
        success = false;
        return QVariant();
    }

    return QVariant(s);
}

/**
 * parseNumber
 */
QVariant Json::parseNumber(const QString &json, int &index)
{
    Json::eatWhitespace(json, index);

    int lastIndex = Json::lastIndexOfNumber(json, index);
    int charLength = (lastIndex - index) + 1;
    QString numberStr;

    numberStr = json.mid(index, charLength);

    index = lastIndex + 1;

    if (numberStr.contains('.')) {
        return QVariant(numberStr.toDouble(NULL));
    } else if (numberStr.startsWith('-')) {
        return QVariant(numberStr.toLongLong(NULL));
    } else {
        return QVariant(numberStr.toULongLong(NULL));
    }
}

/**
 * lastIndexOfNumber
 */
int Json::lastIndexOfNumber(const QString &json, int index)
{
    static const QString numericCharacters("0123456789+-.eE");
    int lastIndex;

    for (lastIndex = index; lastIndex < json.size(); lastIndex++) {
        if (numericCharacters.indexOf(json[lastIndex]) == -1) {
            break;
        }
    }

    return lastIndex - 1;
}

/**
 * eatWhitespace
 */
void Json::eatWhitespace(const QString &json, int &index)
{
    static const QString whitespaceChars(" \t\n\r");
    for (; index < json.size(); index++) {
        if (whitespaceChars.indexOf(json[index]) == -1) {
            break;
        }
    }
}

/**
 * lookAhead
 */
int Json::lookAhead(const QString &json, int index)
{
    int saveIndex = index;
    return Json::nextToken(json, saveIndex);
}

/**
 * nextToken
 */
int Json::nextToken(const QString &json, int &index)
{
    Json::eatWhitespace(json, index);

    if (index == json.size()) {
        return JsonTokenNone;
    }

    QChar c = json[index];
    index++;
    switch (c.toLatin1()) {
        case '{':
            return JsonTokenCurlyOpen;
        case '}':
            return JsonTokenCurlyClose;
        case '[':
            return JsonTokenSquaredOpen;
        case ']':
            return JsonTokenSquaredClose;
        case ',':
            return JsonTokenComma;
        case '"':
            return JsonTokenString;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '-':
            return JsonTokenNumber;
        case ':':
            return JsonTokenColon;
    }

    index--;

    int remainingLength = json.size() - index;

    // True
    if (remainingLength >= 4) {
        if (json[index] == 't' && json[index + 1] == 'r' && json[index + 2] == 'u' && json[index + 3] == 'e') {
            index += 4;
            return JsonTokenTrue;
        }
    }

    // False
    if (remainingLength >= 5) {
        if (json[index] == 'f' && json[index + 1] == 'a' && json[index + 2] == 'l' && json[index + 3] == 's' &&
            json[index + 4] == 'e') {
            index += 5;
            return JsonTokenFalse;
        }
    }

    // Null
    if (remainingLength >= 4) {
        if (json[index] == 'n' && json[index + 1] == 'u' && json[index + 2] == 'l' && json[index + 3] == 'l') {
            index += 4;
            return JsonTokenNull;
        }
    }

    return JsonTokenNone;
}

} // namespace QtJson
