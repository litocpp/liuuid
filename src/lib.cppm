module;
#include <rstd/macro.hpp>
export module liuuid;
export import rstd;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace liuuid
{
enum class ParseErrorKind
{
    Length,
    Character,
    Separator,
    Wrapper
};
struct ParseError {
    ParseErrorKind kind;
    usize          offset {};
};
enum class Variant
{
    Ncs,
    Rfc4122,
    Microsoft,
    Future
};
class Uuid : public DefaultInClass<Uuid, Clone> {
    array<u8, 16> bytes_ {};
    explicit Uuid(array<u8, 16> bytes): bytes_(rstd::move(bytes)) {}
    auto format(bool hyphenated) const -> String {
        constexpr char hex[] = "0123456789abcdef";
        String         text;
        for (usize i {}; i < usize(16); ++i) {
            if (hyphenated && (i == usize(4) || i == usize(6) || i == usize(8) || i == usize(10)))
                text.push_ascii('-');
            auto byte = bytes_[i].to_primitive();
            text.push_ascii(hex[byte >> 4]);
            text.push_ascii(hex[byte & 15]);
        }
        return text;
    }

public:
    Uuid() = default;
    static auto from_bytes(array<u8, 16> bytes) -> Uuid { return Uuid(rstd::move(bytes)); }
    static auto new_v4() -> Result<Uuid, rstd::io::Error> {
        array<u8, 16> bytes {};
        rstd_try(rstd::random::fill_secure(bytes.as_mut_slice()));
        bytes[usize(6)] = (bytes[usize(6)] & u8(15)) | u8(64);
        bytes[usize(8)] = (bytes[usize(8)] & u8(63)) | u8(128);
        return Ok(Uuid(rstd::move(bytes)));
    }
    static auto parse_str(ref<str> input) -> Result<Uuid, ParseError> {
        auto  text = input;
        usize start;
        if (input.len() == usize(38)) {
            if (! input.starts_with("{"_str) || ! input.ends_with("}"_str))
                return Err(ParseError { ParseErrorKind::Wrapper });
            start = usize(1);
            text  = input.get(start, usize(37)).unwrap();
        } else if (input.len() == usize(45)) {
            constexpr char prefix[] = "urn:uuid:";
            for (usize i {}; i < usize(9); ++i) {
                auto byte = input.as_bytes()[i].to_primitive();
                if (byte >= 'A' && byte <= 'Z') byte += 'a' - 'A';
                if (byte != static_cast<unsigned char>(prefix[i.to_primitive()]))
                    return Err(ParseError { ParseErrorKind::Wrapper, i });
            }
            start = usize(9);
            text  = input.get(start, input.len()).unwrap();
        } else if (input.len() != usize(32) && input.len() != usize(36)) {
            return Err(ParseError { ParseErrorKind::Length, input.len() });
        }
        array<u8, 16> bytes {};
        usize         digits;
        unsigned      high {};
        for (usize i {}; i < text.len(); ++i) {
            auto byte = text.as_bytes()[i].to_primitive();
            if (text.len() == usize(36) &&
                (i == usize(8) || i == usize(13) || i == usize(18) || i == usize(23))) {
                if (byte != '-') return Err(ParseError { ParseErrorKind::Separator, start + i });
                continue;
            }
            unsigned value;
            if (byte >= '0' && byte <= '9')
                value = byte - '0';
            else if (byte >= 'a' && byte <= 'f')
                value = byte - 'a' + 10;
            else if (byte >= 'A' && byte <= 'F')
                value = byte - 'A' + 10;
            else
                return Err(ParseError { ParseErrorKind::Character, start + i });
            if (digits % usize(2) == usize())
                high = value << 4;
            else
                bytes[digits / usize(2)] = u8(static_cast<unsigned char>(high | value));
            ++digits;
        }
        return Ok(Uuid(rstd::move(bytes)));
    }
    auto as_bytes() const [[clang::lifetimebound]] -> const array<u8, 16>& { return bytes_; }
    auto version() const -> u8 { return bytes_[usize(6)] >> u64(4); }
    auto variant() const -> Variant {
        auto byte = bytes_[usize(8)];
        if ((byte & u8(0x80)) == u8()) return Variant::Ncs;
        if ((byte & u8(0x40)) == u8()) return Variant::Rfc4122;
        if ((byte & u8(0x20)) == u8()) return Variant::Microsoft;
        return Variant::Future;
    }
    auto simple() const -> String { return format(false); }
    auto hyphenated() const -> String { return format(true); }
    auto clone() const -> Uuid { return *this; }
    auto operator==(const Uuid& other) const -> bool {
        for (usize i {}; i < usize(16); ++i)
            if (bytes_[i] != other.bytes_[i]) return false;
        return true;
    }
};
} // namespace liuuid

export namespace rstd
{
template<>
struct Impl<str_::FromStr, liuuid::Uuid> : ImplBase<liuuid::Uuid> {
    using Err = liuuid::ParseError;
    static auto from_str(ref<str> input) -> Result<liuuid::Uuid, Err> {
        return liuuid::Uuid::parse_str(input);
    }
};
template<>
struct Impl<fmt::Display, liuuid::Uuid> : ImplBase<liuuid::Uuid> {
    auto fmt(fmt::Formatter& f) const -> bool {
        auto text = this->self().hyphenated();
        return f.write_str(text.as_str());
    }
};
template<>
struct Impl<fmt::Debug, liuuid::Uuid> : Impl<fmt::Display, liuuid::Uuid> {};
template<>
struct Impl<fmt::Display, liuuid::ParseError> : ImplBase<liuuid::ParseError> {
    auto fmt(fmt::Formatter& f) const -> bool {
        auto kind = this->self().kind;
        auto text = kind == liuuid::ParseErrorKind::Length      ? "invalid UUID length"_str
                    : kind == liuuid::ParseErrorKind::Character ? "invalid UUID character"_str
                    : kind == liuuid::ParseErrorKind::Separator ? "invalid UUID separator"_str
                                                                : "invalid UUID wrapper"_str;
        return f.write_fmt(fmt::Arguments::make("{} at byte {}", text, this->self().offset));
    }
};
template<>
struct Impl<fmt::Debug, liuuid::ParseError> : Impl<fmt::Display, liuuid::ParseError> {};
template<>
struct Impl<error::Error, liuuid::ParseError> : DefaultInImpl<error::Error, liuuid::ParseError> {};
} // namespace rstd
