#pragma once
// SPDX-License-Identifier: BUSL-1.1
// docs/BrawlerJoinScreen-rationale.md · docs/BrawlerJoinScreen-guards.md

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace brawlerJoinScreen
{

inline constexpr std::size_t kAddressMaxLength = 64u;

constexpr bool isAddressChar(char32_t codePoint)
{
    return (codePoint >= U'0' && codePoint <= U'9')
        || (codePoint >= U'A' && codePoint <= U'Z')
        || (codePoint >= U'a' && codePoint <= U'z')
        || codePoint == U'.'
        || codePoint == U':'
        || codePoint == U'-';
}

constexpr bool isTrimmedWhitespace(char32_t codePoint)
{
    return codePoint == U' ' || codePoint == U'\t' || codePoint == U'\n'
        || codePoint == U'\v' || codePoint == U'\f' || codePoint == U'\r';
}

template <typename CharT>
constexpr char32_t codePointOf(CharT unit)
{
    return static_cast<char32_t>(static_cast<std::make_unsigned_t<CharT>>(unit));
}

template <typename CharT>
constexpr std::basic_string_view<CharT> trimmedText(std::basic_string_view<CharT> text)
{
    std::size_t first = 0u;
    std::size_t last  = text.size();

    while (first < last && isTrimmedWhitespace(codePointOf(text[first])))
        ++first;

    while (last > first && isTrimmedWhitespace(codePointOf(text[last - 1u])))
        --last;

    return text.substr(first, last - first);
}

template <typename CharT>
std::optional<std::string> asciiText(std::basic_string_view<CharT> text)
{
    std::string ascii;
    ascii.reserve(text.size());

    for (const CharT unit : text)
    {
        const char32_t codePoint = codePointOf(unit);
        if (codePoint > U'\x7F')
            return std::nullopt;
        ascii.push_back(static_cast<char>(codePoint));
    }

    return ascii;
}

template <typename CharT>
inline constexpr bool kIsWideCharType = std::is_same_v<CharT, wchar_t>
                                     || std::is_same_v<CharT, char16_t>
                                     || std::is_same_v<CharT, char32_t>;

enum class PasteOutcome : std::uint8_t
{
    Inserted,
    NothingToPaste,
    InvalidCharacter,
    TooLong,
};

class AddressEditBuffer
{
public:
    const std::string& text() const { return m_text; }

    std::size_t cursor() const { return m_cursor; }

    std::string_view textBeforeCursor() const { return std::string_view(m_text).substr(0u, m_cursor); }

    template <typename CharT>
    bool insertTyped(CharT unit)
    {
        static_assert(kIsWideCharType<CharT>,
            "insertTyped takes the platform's wide character (TCHAR, char16_t or char32_t), "
            "never a char. A wide character narrowed to char before the allowed-set test can "
            "turn a rejected character into an accepted one: U+013A narrows to ':' and U+012E "
            "to '.'. See docs/BrawlerJoinScreen-rationale.md section 2.2.");
        return insertCodePoint(codePointOf(unit));
    }

    template <typename CharT>
    PasteOutcome insertPasted(std::basic_string_view<CharT> pasted)
    {
        const std::basic_string_view<CharT> trimmed = trimmedText(pasted);

        if (trimmed.empty())
            return PasteOutcome::NothingToPaste;

        for (const CharT unit : trimmed)
        {
            if (!isAddressChar(codePointOf(unit)))
                return PasteOutcome::InvalidCharacter;
        }

        if (m_text.size() + trimmed.size() > kAddressMaxLength)
            return PasteOutcome::TooLong;

        std::string inserted;
        inserted.reserve(trimmed.size());
        for (const CharT unit : trimmed)
            inserted.push_back(static_cast<char>(codePointOf(unit)));

        m_text.insert(m_cursor, inserted);
        m_cursor += inserted.size();
        return PasteOutcome::Inserted;
    }

    bool backspace()
    {
        if (m_cursor == 0u)
            return false;

        --m_cursor;
        m_text.erase(m_cursor, 1u);
        return true;
    }

    bool deleteForward()
    {
        if (m_cursor >= m_text.size())
            return false;

        m_text.erase(m_cursor, 1u);
        return true;
    }

    bool moveCursorLeft()
    {
        if (m_cursor == 0u)
            return false;

        --m_cursor;
        return true;
    }

    bool moveCursorRight()
    {
        if (m_cursor >= m_text.size())
            return false;

        ++m_cursor;
        return true;
    }

    bool moveCursorToStart()
    {
        const bool moved = m_cursor != 0u;
        m_cursor = 0u;
        return moved;
    }

    bool moveCursorToEnd()
    {
        const bool moved = m_cursor != m_text.size();
        m_cursor = m_text.size();
        return moved;
    }

    bool assign(std::string_view replacement)
    {
        const std::string_view trimmed = trimmedText(replacement);

        if (trimmed.size() > kAddressMaxLength)
            return false;

        for (const char unit : trimmed)
        {
            if (!isAddressChar(codePointOf(unit)))
                return false;
        }

        m_text.assign(trimmed);
        m_cursor = m_text.size();
        return true;
    }

    void clear()
    {
        m_text.clear();
        m_cursor = 0u;
    }

private:
    bool insertCodePoint(char32_t codePoint)
    {
        if (!isAddressChar(codePoint) || m_text.size() >= kAddressMaxLength)
            return false;

        m_text.insert(m_cursor, 1u, static_cast<char>(codePoint));
        ++m_cursor;
        return true;
    }

    std::string m_text;
    std::size_t m_cursor = 0u;
};

inline constexpr std::uint16_t kDefaultServerPort = 7777u;

inline constexpr std::size_t kMaxHostnameLabelLength = 63u;

enum class AddressParseError : std::uint8_t
{
    None,
    Empty,
    TooLong,
    InvalidCharacter,
    MissingHost,
    TooManyColons,
    MissingPort,
    PortNotNumeric,
    PortOutOfRange,
    InvalidIPv4,
    InvalidHostname,
};

enum class HostKind : std::uint8_t
{
    IPv4,
    Hostname,
};

struct ServerAddress
{
    std::string host;

    std::uint16_t port = kDefaultServerPort;

    HostKind kind = HostKind::IPv4;

    std::string canonical() const { return host + ":" + std::to_string(port); }
};

struct ServerAddressParse
{
    AddressParseError error = AddressParseError::Empty;

    ServerAddress address{};

    bool ok() const { return error == AddressParseError::None; }
};

constexpr bool isAsciiDigit(char unit)
{
    return unit >= '0' && unit <= '9';
}

constexpr char asciiLower(char unit)
{
    return (unit >= 'A' && unit <= 'Z') ? static_cast<char>(unit - 'A' + 'a') : unit;
}

inline bool isValidIPv4Host(std::string_view host)
{
    std::size_t octetCount = 0u;
    std::size_t start      = 0u;

    while (true)
    {
        const std::size_t dot   = host.find('.', start);
        const std::string_view octet =
            host.substr(start, (dot == std::string_view::npos) ? std::string_view::npos : dot - start);

        if (octet.empty() || octet.size() > 3u)
            return false;

        if (octet.size() > 1u && octet[0] == '0')
            return false;

        unsigned int value = 0u;
        for (const char unit : octet)
            value = value * 10u + static_cast<unsigned int>(unit - '0');

        if (value > 255u)
            return false;

        ++octetCount;

        if (dot == std::string_view::npos)
            break;

        start = dot + 1u;
    }

    return octetCount == 4u;
}

inline bool isValidHostname(std::string_view host)
{
    std::size_t start = 0u;

    while (true)
    {
        const std::size_t dot   = host.find('.', start);
        const std::string_view label =
            host.substr(start, (dot == std::string_view::npos) ? std::string_view::npos : dot - start);

        if (label.empty() || label.size() > kMaxHostnameLabelLength)
            return false;

        if (label.front() == '-' || label.back() == '-')
            return false;

        if (dot == std::string_view::npos)
            return true;

        start = dot + 1u;
    }
}

inline ServerAddressParse parseServerAddress(std::string_view text)
{
    ServerAddressParse result;

    const std::string_view trimmed = trimmedText(text);

    if (trimmed.empty())
    {
        result.error = AddressParseError::Empty;
        return result;
    }

    if (trimmed.size() > kAddressMaxLength)
    {
        result.error = AddressParseError::TooLong;
        return result;
    }

    for (const char unit : trimmed)
    {
        if (!isAddressChar(codePointOf(unit)))
        {
            result.error = AddressParseError::InvalidCharacter;
            return result;
        }
    }

    const std::size_t colon = trimmed.find(':');

    if (colon != std::string_view::npos && trimmed.find(':', colon + 1u) != std::string_view::npos)
    {
        result.error = AddressParseError::TooManyColons;
        return result;
    }

    const std::string_view host = trimmed.substr(0u, colon);

    if (host.empty())
    {
        result.error = AddressParseError::MissingHost;
        return result;
    }

    std::uint16_t port = kDefaultServerPort;

    if (colon != std::string_view::npos)
    {
        const std::string_view portText = trimmed.substr(colon + 1u);

        if (portText.empty())
        {
            result.error = AddressParseError::MissingPort;
            return result;
        }

        unsigned long value = 0u;
        for (const char unit : portText)
        {
            if (!isAsciiDigit(unit))
            {
                result.error = AddressParseError::PortNotNumeric;
                return result;
            }

            if (value <= 65535u)
                value = value * 10u + static_cast<unsigned long>(unit - '0');
        }

        if (value < 1u || value > 65535u)
        {
            result.error = AddressParseError::PortOutOfRange;
            return result;
        }

        port = static_cast<std::uint16_t>(value);
    }

    bool digitsAndDotsOnly = true;
    for (const char unit : host)
    {
        if (!isAsciiDigit(unit) && unit != '.')
        {
            digitsAndDotsOnly = false;
            break;
        }
    }

    if (digitsAndDotsOnly)
    {
        if (!isValidIPv4Host(host))
        {
            result.error = AddressParseError::InvalidIPv4;
            return result;
        }

        result.address.kind = HostKind::IPv4;
        result.address.host.assign(host);
    }
    else
    {
        if (!isValidHostname(host))
        {
            result.error = AddressParseError::InvalidHostname;
            return result;
        }

        result.address.kind = HostKind::Hostname;
        result.address.host.reserve(host.size());
        for (const char unit : host)
            result.address.host.push_back(asciiLower(unit));
    }

    result.address.port = port;
    result.error        = AddressParseError::None;
    return result;
}

inline std::string_view addressParseErrorText(AddressParseError error)
{
    switch (error)
    {
    case AddressParseError::None:             return "";
    case AddressParseError::Empty:            return "Type a server address, for example 192.168.1.42 or 192.168.1.42:7777.";
    case AddressParseError::TooLong:          return "That address is too long.";
    case AddressParseError::InvalidCharacter: return "An address may only contain letters, digits, '.', ':' and '-'.";
    case AddressParseError::MissingHost:      return "The address is missing the server name or IP before ':'.";
    case AddressParseError::TooManyColons:    return "Only one ':' is allowed (IPv6 addresses are not supported).";
    case AddressParseError::MissingPort:      return "Type a port after ':', or leave out ':' to use port 7777.";
    case AddressParseError::PortNotNumeric:   return "The port must be a number.";
    case AddressParseError::PortOutOfRange:   return "The port must be between 1 and 65535.";
    case AddressParseError::InvalidIPv4:      return "That is not a valid IPv4 address (four numbers 0-255, like 192.168.1.42).";
    case AddressParseError::InvalidHostname:  return "That is not a valid server name.";
    }
    return "";
}

inline constexpr std::string_view kThisPcAddress = "127.0.0.1:7777";

inline constexpr std::string_view kThisPcLabel = "This PC";

inline constexpr std::size_t kRecentAddressCapacity = 5u;

inline constexpr char kRecentAddressSeparator = ',';

static_assert(!isAddressChar(static_cast<char32_t>(kRecentAddressSeparator))
                  && !isTrimmedWhitespace(static_cast<char32_t>(kRecentAddressSeparator)),
    "The recent-address separator must be a character no address can contain and trimming "
    "cannot remove, or one stored address would split into two on the next load. See "
    "docs/BrawlerJoinScreen-rationale.md section 4.2.");

class RecentAddressList
{
public:
    const std::vector<std::string>& entries() const { return m_entries; }

    bool empty() const { return m_entries.empty(); }

    bool remember(std::string_view address)
    {
        const ServerAddressParse parsed = parseServerAddress(address);
        if (!parsed.ok())
            return false;

        const std::string canonical = parsed.address.canonical();
        if (canonical == kThisPcAddress)
            return false;

        for (std::size_t index = 0u; index < m_entries.size(); ++index)
        {
            if (m_entries[index] == canonical)
            {
                if (index == 0u)
                    return false;
                m_entries.erase(m_entries.begin() + static_cast<std::ptrdiff_t>(index));
                break;
            }
        }

        m_entries.insert(m_entries.begin(), canonical);

        if (m_entries.size() > kRecentAddressCapacity)
            m_entries.resize(kRecentAddressCapacity);

        return true;
    }

    std::string serialize() const
    {
        std::string line;
        for (const std::string& entry : m_entries)
        {
            if (!line.empty())
                line.push_back(kRecentAddressSeparator);
            line += entry;
        }
        return line;
    }

    static RecentAddressList deserialize(std::string_view line)
    {
        RecentAddressList list;
        std::size_t start = 0u;

        while (start <= line.size() && list.m_entries.size() < kRecentAddressCapacity)
        {
            const std::size_t separator = line.find(kRecentAddressSeparator, start);
            const std::string_view token = line.substr(
                start, (separator == std::string_view::npos) ? std::string_view::npos : separator - start);

            list.append(token);

            if (separator == std::string_view::npos)
                break;

            start = separator + 1u;
        }

        return list;
    }

private:
    void append(std::string_view address)
    {
        const ServerAddressParse parsed = parseServerAddress(address);
        if (!parsed.ok())
            return;

        const std::string canonical = parsed.address.canonical();
        if (canonical == kThisPcAddress)
            return;

        for (const std::string& entry : m_entries)
        {
            if (entry == canonical)
                return;
        }

        m_entries.push_back(canonical);
    }

    std::vector<std::string> m_entries;
};

inline constexpr std::size_t kJoinListMaxEntries = 1u + kRecentAddressCapacity;

struct JoinListEntry
{
    std::string label;

    std::string address;

    bool isThisPc = false;
};

inline std::vector<JoinListEntry> joinListEntries(const RecentAddressList& recents)
{
    std::vector<JoinListEntry> list;
    list.reserve(kJoinListMaxEntries);

    list.push_back(JoinListEntry{ std::string(kThisPcLabel), std::string(kThisPcAddress), true });

    for (const std::string& entry : recents.entries())
        list.push_back(JoinListEntry{ entry, entry, false });

    return list;
}

template <typename CharT>
std::basic_string_view<CharT> commandLineMapOverrideToken(std::basic_string_view<CharT> commandLine)
{
    std::size_t position = 0u;

    while (position < commandLine.size())
    {
        while (position < commandLine.size() && isTrimmedWhitespace(codePointOf(commandLine[position])))
            ++position;

        if (position >= commandLine.size())
            break;

        std::size_t tokenStart = position;
        std::size_t tokenEnd   = position;

        if (codePointOf(commandLine[position]) == U'"')
        {
            tokenStart = position + 1u;
            tokenEnd   = tokenStart;
            while (tokenEnd < commandLine.size() && codePointOf(commandLine[tokenEnd]) != U'"')
                ++tokenEnd;
            position = (tokenEnd < commandLine.size()) ? tokenEnd + 1u : tokenEnd;
        }
        else
        {
            bool inQuote = false;
            while (tokenEnd < commandLine.size())
            {
                const char32_t codePoint = codePointOf(commandLine[tokenEnd]);
                if (!inQuote && isTrimmedWhitespace(codePoint))
                    break;
                if (codePoint == U'"')
                    inQuote = !inQuote;
                ++tokenEnd;
            }
            position = tokenEnd;
        }

        const std::basic_string_view<CharT> token = commandLine.substr(tokenStart, tokenEnd - tokenStart);

        if (token.empty())
            continue;

        if (codePointOf(token[0]) != U'-')
            return token;

        constexpr std::string_view kMapPrefix = "-map=";
        if (token.size() >= kMapPrefix.size())
        {
            bool prefixMatches = true;
            for (std::size_t index = 0u; index < kMapPrefix.size(); ++index)
            {
                const char32_t codePoint = codePointOf(token[index]);
                const char32_t lowered =
                    (codePoint >= U'A' && codePoint <= U'Z') ? static_cast<char32_t>(codePoint - U'A' + U'a') : codePoint;
                if (lowered != static_cast<char32_t>(kMapPrefix[index]))
                {
                    prefixMatches = false;
                    break;
                }
            }
            if (prefixMatches)
                return token.substr(kMapPrefix.size());
        }
    }

    return {};
}

enum class JoinFailureReason : std::uint8_t
{
    CannotReachServer,
    DifferentBuild,
    ConnectionLost,
    ServerRefused,
    Unknown,
};

inline constexpr std::size_t kJoinFailureReasonCount = 5u;

static_assert(static_cast<std::size_t>(JoinFailureReason::Unknown) + 1u == kJoinFailureReasonCount,
    "kJoinFailureReasonCount must count every JoinFailureReason, and Unknown stays last: the test "
    "suite walks 0..count-1 to prove each reason has its own non-empty headline. A reason "
    "appended after Unknown would be skipped by that walk.");

inline constexpr std::string_view kDevBuildLabel = "dev";

struct JoinStatusText
{
    std::string headline;

    std::string detail;
};

inline JoinStatusText joinFailureText(JoinFailureReason reason,
                                      std::string_view  ownBuildLabel,
                                      std::string_view  serverText,
                                      std::string_view  address)
{
    switch (reason)
    {
    case JoinFailureReason::CannotReachServer:
        return { "Can't reach server " + std::string(address) + ".",
                 "Check the address, and that the server is running and reachable." };
    case JoinFailureReason::DifferentBuild:
        return { "Different build.",
                 "This game is " + std::string(ownBuildLabel.empty() ? kDevBuildLabel : ownBuildLabel)
                     + "; the server runs a different build." };
    case JoinFailureReason::ConnectionLost:
        return { "Connection lost.", "The server stopped responding." };
    case JoinFailureReason::ServerRefused:
        return { "Server refused: " + (serverText.empty() ? std::string("no reason given") : std::string(serverText)),
                 "" };
    case JoinFailureReason::Unknown:
        return { "Could not join.", serverText.empty() ? std::string("Unknown error.") : std::string(serverText) };
    }
    return { "Could not join.", "Unknown error." };
}

struct LocalCoopKeyNames
{
    std::string_view addPlayer;

    std::string_view removePlayer;
};

// ⛔G-01  docs/BrawlerJoinScreen-guards.md
inline constexpr LocalCoopKeyNames kLocalCoopKeyNames{ "Tab", "Insert" };

inline std::string localCoopHintText()
{
    return "After joining: " + std::string(kLocalCoopKeyNames.addPlayer) + " adds a local player, "
         + std::string(kLocalCoopKeyNames.removePlayer) + " removes one";
}

inline std::string localPlayerLimitNoticeText(int maxLocalPlayersPerClient)
{
    return "No local player added: this PC already has " + std::to_string(maxLocalPlayersPerClient)
         + ", the most one PC can have.";
}

inline constexpr float kLocalPlayerLimitNoticeSeconds = 4.f;

constexpr bool localPlayerLimitNoticeVisible(float secondsSinceRefused)
{
    return secondsSinceRefused >= 0.f && secondsSinceRefused < kLocalPlayerLimitNoticeSeconds;
}

inline constexpr std::string_view kJoinScreenTitle = "OGBrawler - join a server";

inline constexpr std::string_view kJoinButtonLabel = "Join";

inline std::string buildLabelLine(std::string_view ownBuildLabel)
{
    return "Build: " + std::string(ownBuildLabel.empty() ? kDevBuildLabel : ownBuildLabel);
}

enum class JoinFocusArea : std::uint8_t
{
    List,
    Field,
    JoinButton,
};

struct JoinFocus
{
    JoinFocusArea area = JoinFocusArea::List;

    std::size_t listIndex = 0u;
};

enum class JoinNavigation : std::uint8_t
{
    Up,
    Down,
};

inline JoinFocus navigatedFocus(JoinFocus focus, JoinNavigation direction, std::size_t listCount)
{
    const std::size_t lastListIndex = (listCount == 0u) ? 0u : listCount - 1u;

    JoinFocus next = focus;
    if (next.listIndex > lastListIndex)
        next.listIndex = lastListIndex;

    if (direction == JoinNavigation::Down)
    {
        switch (focus.area)
        {
        case JoinFocusArea::List:
            if (next.listIndex < lastListIndex)
                ++next.listIndex;
            else
                next.area = JoinFocusArea::Field;
            break;
        case JoinFocusArea::Field:
            next.area = JoinFocusArea::JoinButton;
            break;
        case JoinFocusArea::JoinButton:
            break;
        }
        return next;
    }

    switch (focus.area)
    {
    case JoinFocusArea::List:
        if (next.listIndex > 0u)
            --next.listIndex;
        break;
    case JoinFocusArea::Field:
        if (listCount > 0u)
        {
            next.area      = JoinFocusArea::List;
            next.listIndex = lastListIndex;
        }
        break;
    case JoinFocusArea::JoinButton:
        next.area = JoinFocusArea::Field;
        break;
    }
    return next;
}

enum class JoinPhase : std::uint8_t
{
    Editing,
    Connecting,
    Joined,
    Failed,
};

enum class JoinStatusTone : std::uint8_t
{
    Prompt,
    Progress,
    Error,
};

struct JoinStatusLine
{
    JoinStatusTone tone = JoinStatusTone::Prompt;

    JoinStatusText text{};
};

inline constexpr std::string_view kEditingPrompt = "Type an address, or pick one above, then press Join.";

class JoinScreenModel
{
public:
    JoinScreenModel() = default;

    static JoinScreenModel start(RecentAddressList recents,
                                 std::string_view  commandLineAddress,
                                 bool              reachedViaFailureReturn)
    {
        JoinScreenModel model;
        model.m_recents = std::move(recents);

        if (!model.m_recents.empty())
        {
            model.m_field.assign(model.m_recents.entries().front());
            model.m_focus = JoinFocus{ JoinFocusArea::Field, 0u };
        }
        else
        {
            model.m_focus = JoinFocus{ JoinFocusArea::List, 0u };
        }

        if (reachedViaFailureReturn || trimmedText(commandLineAddress).empty())
            return model;

        const ServerAddressParse parsed = parseServerAddress(commandLineAddress);
        if (!parsed.ok())
        {
            model.m_notice = "Ignored the address on the command line: "
                           + std::string(addressParseErrorText(parsed.error));
            return model;
        }

        model.m_field.assign(parsed.address.canonical());
        model.m_focus = JoinFocus{ JoinFocusArea::Field, 0u };
        model.beginConnecting(parsed.address.canonical());
        return model;
    }

    JoinPhase phase() const { return m_phase; }

    const AddressEditBuffer& field() const { return m_field; }

    const RecentAddressList& recents() const { return m_recents; }

    JoinFocus focus() const { return m_focus; }

    const std::string& targetAddress() const { return m_targetAddress; }

    JoinFailureReason failureReason() const { return m_failureReason; }

    const std::string& failureServerText() const { return m_failureServerText; }

    AddressParseError rejectedFieldError() const { return m_rejectedFieldError; }

    const std::string& notice() const { return m_notice; }

    bool acceptsInput() const { return m_phase == JoinPhase::Editing || m_phase == JoinPhase::Failed; }

    std::vector<JoinListEntry> listEntries() const { return joinListEntries(m_recents); }

    template <typename CharT>
    bool typeCharacter(CharT unit)
    {
        return editField([unit](AddressEditBuffer& buffer) { return buffer.insertTyped(unit); });
    }

    template <typename CharT>
    PasteOutcome paste(std::basic_string_view<CharT> pasted)
    {
        PasteOutcome outcome = PasteOutcome::NothingToPaste;
        editField([&outcome, pasted](AddressEditBuffer& buffer)
        {
            outcome = buffer.insertPasted(pasted);
            return outcome == PasteOutcome::Inserted;
        });
        return outcome;
    }

    bool backspace()
    {
        return editField([](AddressEditBuffer& buffer) { return buffer.backspace(); });
    }

    bool deleteForward()
    {
        return editField([](AddressEditBuffer& buffer) { return buffer.deleteForward(); });
    }

    bool moveCursorLeft()
    {
        return editField([](AddressEditBuffer& buffer) { return buffer.moveCursorLeft(); });
    }

    bool moveCursorRight()
    {
        return editField([](AddressEditBuffer& buffer) { return buffer.moveCursorRight(); });
    }

    bool moveCursorToStart()
    {
        return editField([](AddressEditBuffer& buffer) { return buffer.moveCursorToStart(); });
    }

    bool moveCursorToEnd()
    {
        return editField([](AddressEditBuffer& buffer) { return buffer.moveCursorToEnd(); });
    }

    bool navigate(JoinNavigation direction)
    {
        if (!acceptsInput())
            return false;

        const JoinFocus next = navigatedFocus(m_focus, direction, listEntries().size());
        const bool moved = next.area != m_focus.area || next.listIndex != m_focus.listIndex;
        m_focus = next;
        return moved;
    }

    std::optional<std::string> activate()
    {
        if (!acceptsInput())
            return std::nullopt;

        if (m_focus.area == JoinFocusArea::List)
        {
            const std::vector<JoinListEntry> list = listEntries();
            const std::size_t index = (m_focus.listIndex < list.size()) ? m_focus.listIndex : list.size() - 1u;
            m_field.assign(list[index].address);
        }

        return requestJoin(m_field.text());
    }

    std::optional<std::string> requestJoin(std::string_view address)
    {
        if (!acceptsInput())
            return std::nullopt;

        const ServerAddressParse parsed = parseServerAddress(address);
        if (!parsed.ok())
        {
            m_rejectedFieldError = parsed.error;
            return std::nullopt;
        }

        beginConnecting(parsed.address.canonical());
        return m_targetAddress;
    }

    bool cancel()
    {
        if (m_phase != JoinPhase::Connecting)
            return false;

        m_phase = JoinPhase::Editing;
        return true;
    }

    bool noteJoinSucceeded(bool rememberAddress)
    {
        if (m_phase != JoinPhase::Connecting)
            return false;

        m_phase = JoinPhase::Joined;
        return rememberAddress && m_recents.remember(m_targetAddress);
    }

    bool noteJoinFailed(JoinFailureReason reason, std::string_view serverText)
    {
        if (m_phase != JoinPhase::Connecting && m_phase != JoinPhase::Joined)
            return false;

        m_phase             = JoinPhase::Failed;
        m_failureReason     = reason;
        m_failureServerText.assign(serverText);
        return true;
    }

    JoinStatusLine statusLine(std::string_view ownBuildLabel) const
    {
        switch (m_phase)
        {
        case JoinPhase::Connecting:
            return { JoinStatusTone::Progress, { "Connecting to " + m_targetAddress + "...", "" } };
        case JoinPhase::Failed:
            return { JoinStatusTone::Error,
                     joinFailureText(m_failureReason, ownBuildLabel, m_failureServerText, m_targetAddress) };
        case JoinPhase::Joined:
            return { JoinStatusTone::Progress, { "Joined " + m_targetAddress + ".", "" } };
        case JoinPhase::Editing:
            break;
        }

        if (m_rejectedFieldError != AddressParseError::None)
            return { JoinStatusTone::Error, { std::string(addressParseErrorText(m_rejectedFieldError)), "" } };

        return { JoinStatusTone::Prompt, { std::string(kEditingPrompt), m_notice } };
    }

    template <typename Edit>
    bool editField(Edit&& edit)
    {
        if (!acceptsInput())
            return false;

        if (!edit(m_field))
            return false;

        m_focus              = JoinFocus{ JoinFocusArea::Field, m_focus.listIndex };
        m_rejectedFieldError = AddressParseError::None;
        m_notice.clear();
        if (m_phase == JoinPhase::Failed)
            m_phase = JoinPhase::Editing;
        return true;
    }

    void beginConnecting(std::string canonicalAddress)
    {
        m_targetAddress      = std::move(canonicalAddress);
        m_phase              = JoinPhase::Connecting;
        m_rejectedFieldError = AddressParseError::None;
        m_notice.clear();
        m_failureServerText.clear();
    }

    AddressEditBuffer m_field;
    RecentAddressList m_recents;
    JoinFocus         m_focus{};
    JoinPhase         m_phase = JoinPhase::Editing;
    std::string       m_targetAddress;
    JoinFailureReason m_failureReason = JoinFailureReason::Unknown;
    std::string       m_failureServerText;
    AddressParseError m_rejectedFieldError = AddressParseError::None;
    std::string       m_notice;
};

struct JoinScreenInk
{
    float r = 1.f;
    float g = 1.f;
    float b = 1.f;
};

static_assert(sizeof(JoinScreenInk) == 3u * sizeof(float),
    "JoinScreenInk is exactly three linear-0..1 floats, like the scoreboard's ink. The panel's "
    "only translucency is kJoinScreenBackdropAlpha; a per-ink alpha would let the game world "
    "behind the panel change how legible the address and the failure text are.");

inline constexpr JoinScreenInk kJoinScreenTextInk{ 1.f, 1.f, 1.f };
inline constexpr JoinScreenInk kJoinScreenDimTextInk{ 0.62f, 0.62f, 0.66f };
inline constexpr JoinScreenInk kJoinScreenFocusInk{ 0.98f, 0.78f, 0.25f };
inline constexpr JoinScreenInk kJoinScreenErrorInk{ 0.95f, 0.42f, 0.36f };
inline constexpr JoinScreenInk kJoinScreenProgressInk{ 0.55f, 0.85f, 0.95f };
inline constexpr JoinScreenInk kJoinScreenFieldInk{ 0.12f, 0.12f, 0.15f };
inline constexpr JoinScreenInk kJoinScreenBackdropInk{ 0.f, 0.f, 0.f };

inline constexpr float kJoinScreenBackdropAlpha = 0.8f;

constexpr JoinScreenInk joinStatusInk(JoinStatusTone tone)
{
    switch (tone)
    {
    case JoinStatusTone::Prompt:   return kJoinScreenDimTextInk;
    case JoinStatusTone::Progress: return kJoinScreenProgressInk;
    case JoinStatusTone::Error:    return kJoinScreenErrorInk;
    }
    return kJoinScreenTextInk;
}

inline constexpr float kJoinScreenDefaultScale = 1.f;
inline constexpr float kJoinScreenMinScale     = 0.5f;
inline constexpr float kJoinScreenMaxScale     = 4.f;

inline constexpr float kJoinScreenReferenceCanvasHeight = 720.f;

inline constexpr float kJoinScreenTitleTextScale = 1.4f;

constexpr float clampJoinScreenScale(float requestedScale)
{
    if (!(requestedScale >= kJoinScreenMinScale))
        return kJoinScreenMinScale;

    if (requestedScale > kJoinScreenMaxScale)
        return kJoinScreenMaxScale;

    return requestedScale;
}

struct JoinScreenRect
{
    float x      = 0.f;
    float y      = 0.f;
    float width  = 0.f;
    float height = 0.f;
};

struct JoinScreenMetrics
{
    float panelWidth   = 560.f;
    float padding      = 16.f;
    float titleHeight  = 26.f;
    float lineHeight   = 18.f;
    float rowHeight    = 20.f;
    float fieldHeight  = 24.f;
    float buttonWidth  = 96.f;
    float buttonHeight = 24.f;
    float gap          = 8.f;
    float textInsetX   = 6.f;
    float textOffsetY  = 3.f;
};

struct JoinScreenLayout
{
    float scale = 1.f;

    float titleTextScale = 1.f;

    float textInsetX = 0.f;

    float textOffsetY = 0.f;

    JoinScreenRect panel{};
    JoinScreenRect title{};
    JoinScreenRect buildLabel{};

    std::array<JoinScreenRect, kJoinListMaxEntries> listRows{};

    std::size_t listRowCount = 0u;

    JoinScreenRect field{};
    JoinScreenRect joinButton{};
    JoinScreenRect statusHeadline{};
    JoinScreenRect statusDetail{};
    JoinScreenRect hint{};
};

inline float joinScreenBaseHeight(const JoinScreenMetrics& metrics, std::size_t listRowCount)
{
    const std::size_t rows = (listRowCount < kJoinListMaxEntries) ? listRowCount : kJoinListMaxEntries;

    return metrics.padding
         + metrics.titleHeight
         + metrics.lineHeight
         + metrics.gap
         + static_cast<float>(rows) * metrics.rowHeight
         + metrics.gap
         + metrics.fieldHeight
         + metrics.gap
         + metrics.buttonHeight
         + metrics.gap
         + 3.f * metrics.lineHeight
         + metrics.padding;
}

inline float joinScreenEffectiveScale(const JoinScreenMetrics& metrics,
                                      std::size_t              listRowCount,
                                      float                    requestedScale,
                                      float                    canvasWidth,
                                      float                    canvasHeight)
{
    const float clamped = clampJoinScreenScale(requestedScale);

    if (!(canvasWidth > 0.f) || !(canvasHeight > 0.f))
        return clamped;

    const float resolutionScaled = clamped * (canvasHeight / kJoinScreenReferenceCanvasHeight);

    const float fitWidth  = canvasWidth / metrics.panelWidth;
    const float fitHeight = canvasHeight / joinScreenBaseHeight(metrics, listRowCount);
    const float fit       = (fitWidth < fitHeight) ? fitWidth : fitHeight;

    return (resolutionScaled < fit) ? resolutionScaled : fit;
}

inline JoinScreenLayout placedJoinScreenLayout(std::size_t              listRowCount,
                                               float                    requestedScale,
                                               float                    canvasWidth,
                                               float                    canvasHeight,
                                               const JoinScreenMetrics& metrics = JoinScreenMetrics{})
{
    JoinScreenLayout layout;

    const float scale =
        joinScreenEffectiveScale(metrics, listRowCount, requestedScale, canvasWidth, canvasHeight);

    layout.scale        = scale;
    layout.titleTextScale = scale * kJoinScreenTitleTextScale;
    layout.textInsetX   = metrics.textInsetX * scale;
    layout.textOffsetY  = metrics.textOffsetY * scale;
    layout.listRowCount = (listRowCount < kJoinListMaxEntries) ? listRowCount : kJoinListMaxEntries;

    const float panelWidth  = metrics.panelWidth * scale;
    const float panelHeight = joinScreenBaseHeight(metrics, layout.listRowCount) * scale;

    const float originX = (canvasWidth > panelWidth) ? (canvasWidth - panelWidth) * 0.5f : 0.f;
    const float originY = (canvasHeight > panelHeight) ? (canvasHeight - panelHeight) * 0.5f : 0.f;

    layout.panel = JoinScreenRect{ originX, originY, panelWidth, panelHeight };

    const float padding    = metrics.padding * scale;
    const float innerX     = originX + padding;
    const float innerWidth = panelWidth - 2.f * padding;
    const float gap        = metrics.gap * scale;
    const float lineHeight = metrics.lineHeight * scale;

    float cursorY = originY + padding;

    layout.title = JoinScreenRect{ innerX, cursorY, innerWidth, metrics.titleHeight * scale };
    cursorY += layout.title.height;

    layout.buildLabel = JoinScreenRect{ innerX, cursorY, innerWidth, lineHeight };
    cursorY += lineHeight + gap;

    const float rowHeight = metrics.rowHeight * scale;
    for (std::size_t row = 0u; row < layout.listRowCount; ++row)
    {
        layout.listRows[row] = JoinScreenRect{ innerX, cursorY, innerWidth, rowHeight };
        cursorY += rowHeight;
    }
    cursorY += gap;

    layout.field = JoinScreenRect{ innerX, cursorY, innerWidth, metrics.fieldHeight * scale };
    cursorY += layout.field.height + gap;

    layout.joinButton = JoinScreenRect{ innerX, cursorY, metrics.buttonWidth * scale, metrics.buttonHeight * scale };
    cursorY += layout.joinButton.height + gap;

    layout.statusHeadline = JoinScreenRect{ innerX, cursorY, innerWidth, lineHeight };
    cursorY += lineHeight;

    layout.statusDetail = JoinScreenRect{ innerX, cursorY, innerWidth, lineHeight };
    cursorY += lineHeight;

    layout.hint = JoinScreenRect{ innerX, cursorY, innerWidth, lineHeight };

    return layout;
}

} // namespace brawlerJoinScreen
