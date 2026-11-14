#include <rstd/test/gtest.hpp>
import liuuid;
import rstd.test;
using namespace rstd::prelude;
using namespace rstd::literals;
using liuuid::Uuid;

TEST(Uuid, ParsesRustTextForms) {
    auto     expected = Uuid::parse_str("67e55044-10b1-426f-9247-bb680e5fe0c8"_str).unwrap();
    ref<str> forms[] { "67e5504410b1426f9247bb680e5fe0c8"_str,
                       "67E55044-10B1-426F-9247-BB680E5FE0C8"_str,
                       "{67e55044-10b1-426f-9247-bb680e5fe0c8}"_str,
                       "urn:uuid:67e55044-10b1-426f-9247-bb680e5fe0c8"_str,
                       "URN:UUID:67E55044-10B1-426F-9247-BB680E5FE0C8"_str };
    for (auto form : forms) EXPECT_TRUE(Uuid::parse_str(form).unwrap() == expected);
    EXPECT_EQ(expected.hyphenated().as_str(), "67e55044-10b1-426f-9247-bb680e5fe0c8"_str);
    EXPECT_EQ(expected.simple().as_str(), "67e5504410b1426f9247bb680e5fe0c8"_str);
    EXPECT_TRUE(rstd::from_str<Uuid>(forms[0]).unwrap() == expected);
    EXPECT_EQ(rstd::format("{}", expected).as_str(), expected.hyphenated().as_str());
    EXPECT_EQ(rstd::format("{:?}", expected).as_str(), expected.hyphenated().as_str());
}
TEST(Uuid, RejectsMalformedText) {
    ref<str> invalid[] { ""_str,
                         "67e5504410b1426f9247bb680e5fe0c"_str,
                         "67e5504410b1426f9247bb680e5fe0c80"_str,
                         "67e55044_10b1-426f-9247-bb680e5fe0c8"_str,
                         "67e55044-10b1-426f-9247-bb680e5fe0cg"_str,
                         "{67e5504410b1426f9247bb680e5fe0c8}"_str,
                         "urn:uuid:67e5504410b1426f9247bb680e5fe0c8"_str,
                         "urn:uuid:{67e55044-10b1-426f-9247-bb680e5fe0c8}"_str,
                         " 67e55044-10b1-426f-9247-bb680e5fe0c8"_str,
                         "[67e55044-10b1-426f-9247-bb680e5fe0c8]"_str,
                         "urn:guid:67e55044-10b1-426f-9247-bb680e5fe0c8"_str };
    for (auto text : invalid) EXPECT_TRUE(Uuid::parse_str(text).is_err());
    auto error = Uuid::parse_str(invalid[3]).unwrap_err();
    EXPECT_EQ(error.kind, liuuid::ParseErrorKind::Separator);
    EXPECT_EQ(error.offset, usize(8));
}
TEST(Uuid, OwnsBytesWithoutChangingVersion) {
    array<u8, 16> bytes {};
    bytes[usize()]   = u8(0xab);
    bytes[usize(6)]  = u8(0x12);
    bytes[usize(8)]  = u8(0xfe);
    bytes[usize(15)] = u8(0xcd);
    auto value       = Uuid::from_bytes(bytes);
    EXPECT_EQ(value.simple().as_str(), "ab00000000001200fe000000000000cd"_str);
    EXPECT_TRUE(value == value.clone());
    EXPECT_TRUE(Uuid::from_bytes(value.as_bytes()) == value);
    EXPECT_TRUE(Uuid::parse_str(value.hyphenated().as_str()).unwrap() == value);
    EXPECT_EQ(Uuid().simple().as_str(), "00000000000000000000000000000000"_str);
}
TEST(Uuid, GeneratesV4Values) {
    auto first = Uuid::new_v4().unwrap();
    for (unsigned i {}; i < 16; ++i) {
        auto value = Uuid::new_v4().unwrap();
        EXPECT_FALSE(value == first);
        EXPECT_EQ(value.version(), u8(4));
        EXPECT_EQ(value.variant(), liuuid::Variant::Rfc4122);
        EXPECT_EQ(value.as_bytes()[usize(6)] & u8(0xf0), u8(0x40));
        EXPECT_EQ(value.as_bytes()[usize(8)] & u8(0xc0), u8(0x80));
        EXPECT_EQ(value.simple().len(), usize(32));
        EXPECT_EQ(value.hyphenated().len(), usize(36));
        EXPECT_TRUE(Uuid::parse_str(value.hyphenated().as_str()).unwrap() == value);
    }
}
TEST(Uuid, ReportsVersionAndVariantWithoutRestrictingParsing) {
    EXPECT_EQ(Uuid().version(), u8());
    EXPECT_EQ(Uuid().variant(), liuuid::Variant::Ncs);
    auto value = Uuid::parse_str("67e55044-10b1-126f-9247-bb680e5fe0c8"_str).unwrap();
    EXPECT_EQ(value.version(), u8(1));
    EXPECT_EQ(value.variant(), liuuid::Variant::Rfc4122);
    EXPECT_EQ(Uuid::parse_str("67e55044-10b1-f26f-c247-bb680e5fe0c8"_str).unwrap().variant(),
              liuuid::Variant::Microsoft);
    EXPECT_EQ(Uuid::parse_str("67e55044-10b1-f26f-e247-bb680e5fe0c8"_str).unwrap().variant(),
              liuuid::Variant::Future);
}
