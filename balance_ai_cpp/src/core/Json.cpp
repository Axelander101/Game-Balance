#include "gba/core/Json.h"
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace gba {
namespace {
const Json kNull;

std::string escapeString(const std::string& s) {
    std::ostringstream out;
    for (unsigned char c : s) {
        switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (c < 0x20) out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << int(c) << std::dec;
                else out << char(c);
        }
    }
    return out.str();
}

class Parser {
public:
    explicit Parser(const std::string& s) : s_(s) {}
    Json parse() {
        skip();
        Json j = value();
        skip();
        if (pos_ != s_.size()) error("unerwartete Zeichen am Ende");
        return j;
    }
private:
    const std::string& s_;
    std::size_t pos_ = 0;

    [[noreturn]] void error(const std::string& msg) const {
        throw std::runtime_error("JSON Fehler bei Position " + std::to_string(pos_) + ": " + msg);
    }
    void skip() { while (pos_ < s_.size() && std::isspace(static_cast<unsigned char>(s_[pos_]))) ++pos_; }
    bool consume(char c) { skip(); if (pos_ < s_.size() && s_[pos_] == c) { ++pos_; return true; } return false; }
    Json value() {
        skip();
        if (pos_ >= s_.size()) error("unerwartetes Dateiende");
        char c = s_[pos_];
        if (c == '{') return object();
        if (c == '[') return array();
        if (c == '"') return Json(string());
        if (c == 't') { literal("true"); return Json(true); }
        if (c == 'f') { literal("false"); return Json(false); }
        if (c == 'n') { literal("null"); return Json(nullptr); }
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return Json(number());
        error("ungueltiger Wert");
    }
    void literal(const char* t) {
        for (std::size_t i = 0; t[i]; ++i) if (pos_ + i >= s_.size() || s_[pos_ + i] != t[i]) error("ungueltiges Literal");
        pos_ += std::char_traits<char>::length(t);
    }
    std::string string() {
        if (!consume('"')) error("String erwartet");
        std::string out;
        while (pos_ < s_.size()) {
            char c = s_[pos_++];
            if (c == '"') return out;
            if (c == '\\') {
                if (pos_ >= s_.size()) error("ungueltiger Escape");
                char e = s_[pos_++];
                switch (e) {
                    case '"': out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break;
                    case 't': out.push_back('\t'); break;
                    case 'u': {
                        if (pos_ + 4 > s_.size()) error("ungueltiger Unicode Escape");
                        unsigned code = 0;
                        for (int i = 0; i < 4; ++i) {
                            char h = s_[pos_++];
                            code <<= 4;
                            if (h >= '0' && h <= '9') code += h - '0';
                            else if (h >= 'a' && h <= 'f') code += 10 + h - 'a';
                            else if (h >= 'A' && h <= 'F') code += 10 + h - 'A';
                            else error("ungueltiger Unicode Escape");
                        }
                        if (code <= 0x7F) out.push_back(char(code));
                        else if (code <= 0x7FF) { out.push_back(char(0xC0 | (code >> 6))); out.push_back(char(0x80 | (code & 0x3F))); }
                        else { out.push_back(char(0xE0 | (code >> 12))); out.push_back(char(0x80 | ((code >> 6) & 0x3F))); out.push_back(char(0x80 | (code & 0x3F))); }
                        break;
                    }
                    default: error("unbekannter Escape");
                }
            } else out.push_back(c);
        }
        error("nicht geschlossener String");
    }
    double number() {
        skip();
        std::size_t start = pos_;
        if (s_[pos_] == '-') ++pos_;
        if (pos_ >= s_.size()) error("ungueltige Zahl");
        if (s_[pos_] == '0') ++pos_;
        else {
            if (!std::isdigit(static_cast<unsigned char>(s_[pos_]))) error("ungueltige Zahl");
            while (pos_ < s_.size() && std::isdigit(static_cast<unsigned char>(s_[pos_]))) ++pos_;
        }
        if (pos_ < s_.size() && s_[pos_] == '.') {
            ++pos_;
            if (pos_ >= s_.size() || !std::isdigit(static_cast<unsigned char>(s_[pos_]))) error("ungueltige Dezimalzahl");
            while (pos_ < s_.size() && std::isdigit(static_cast<unsigned char>(s_[pos_]))) ++pos_;
        }
        if (pos_ < s_.size() && (s_[pos_] == 'e' || s_[pos_] == 'E')) {
            ++pos_;
            if (pos_ < s_.size() && (s_[pos_] == '+' || s_[pos_] == '-')) ++pos_;
            if (pos_ >= s_.size() || !std::isdigit(static_cast<unsigned char>(s_[pos_]))) error("ungueltiger Exponent");
            while (pos_ < s_.size() && std::isdigit(static_cast<unsigned char>(s_[pos_]))) ++pos_;
        }
        return std::stod(s_.substr(start, pos_ - start));
    }
    Json array() {
        consume('['); Json::Array a; skip();
        if (consume(']')) return Json(std::move(a));
        while (true) {
            a.push_back(value()); skip();
            if (consume(']')) break;
            if (!consume(',')) error("',' oder ']' erwartet");
        }
        return Json(std::move(a));
    }
    Json object() {
        consume('{'); Json::Object o; skip();
        if (consume('}')) return Json(std::move(o));
        while (true) {
            skip(); if (pos_ >= s_.size() || s_[pos_] != '"') error("Objektschluessel erwartet");
            std::string k = string();
            if (!consume(':')) error("':' erwartet");
            o[k] = value();
            if (consume('}')) break;
            if (!consume(',')) error("',' oder '}' erwartet");
        }
        return Json(std::move(o));
    }
};

void dumpImpl(const Json& j, std::ostringstream& out, int indent, int depth) {
    auto pad = [&](int d) { for (int i = 0; i < d * indent; ++i) out << ' '; };
    if (j.isNull()) { out << "null"; return; }
    if (j.isBool()) { out << (j.asBool() ? "true" : "false"); return; }
    if (j.isNumber()) {
        double v = j.asNumber();
        if (std::fabs(v - std::round(v)) < 1e-12 && std::fabs(v) < 9e15) out << std::fixed << std::setprecision(0) << v << std::defaultfloat;
        else out << std::setprecision(15) << v;
        return;
    }
    if (j.isString()) { out << '"' << escapeString(j.asString()) << '"'; return; }
    if (j.isArray()) {
        out << '['; const auto& a = j.asArray();
        if (!a.empty()) {
            for (std::size_t i = 0; i < a.size(); ++i) {
                if (indent > 0) { out << '\n'; pad(depth + 1); }
                dumpImpl(a[i], out, indent, depth + 1);
                if (i + 1 < a.size()) out << ',';
            }
            if (indent > 0) { out << '\n'; pad(depth); }
        }
        out << ']'; return;
    }
    out << '{'; const auto& o = j.asObject();
    if (!o.empty()) {
        std::size_t i = 0;
        for (const auto& [k, v] : o) {
            if (indent > 0) { out << '\n'; pad(depth + 1); }
            out << '"' << escapeString(k) << "\":"; if (indent > 0) out << ' ';
            dumpImpl(v, out, indent, depth + 1);
            if (++i < o.size()) out << ',';
        }
        if (indent > 0) { out << '\n'; pad(depth); }
    }
    out << '}';
}
}

Json::Json() : value_(nullptr) {}
Json::Json(std::nullptr_t) : value_(nullptr) {}
Json::Json(bool v) : value_(v) {}
Json::Json(int v) : value_(double(v)) {}
Json::Json(long long v) : value_(double(v)) {}
Json::Json(double v) : value_(v) {}
Json::Json(const char* v) : value_(std::string(v)) {}
Json::Json(std::string v) : value_(std::move(v)) {}
Json::Json(Array v) : value_(std::move(v)) {}
Json::Json(Object v) : value_(std::move(v)) {}

Json Json::parse(const std::string& text) { return Parser(text).parse(); }
Json Json::parseFile(const std::string& path) { std::ifstream f(path, std::ios::binary); if (!f) throw std::runtime_error("Kann JSON-Datei nicht oeffnen: " + path); std::ostringstream s; s << f.rdbuf(); return parse(s.str()); }
void Json::writeFile(const std::string& path, int indent) const { std::ofstream f(path, std::ios::binary); if (!f) throw std::runtime_error("Kann JSON-Datei nicht schreiben: " + path); f << dump(indent) << '\n'; }
std::string Json::dump(int indent) const { std::ostringstream out; dumpImpl(*this, out, indent, 0); return out.str(); }

bool Json::isNull() const { return std::holds_alternative<std::nullptr_t>(value_); }
bool Json::isBool() const { return std::holds_alternative<bool>(value_); }
bool Json::isNumber() const { return std::holds_alternative<double>(value_); }
bool Json::isString() const { return std::holds_alternative<std::string>(value_); }
bool Json::isArray() const { return std::holds_alternative<Array>(value_); }
bool Json::isObject() const { return std::holds_alternative<Object>(value_); }
bool Json::asBool(bool fallback) const { return isBool() ? std::get<bool>(value_) : fallback; }
double Json::asNumber(double fallback) const { return isNumber() ? std::get<double>(value_) : fallback; }
int Json::asInt(int fallback) const { return isNumber() ? int(std::llround(std::get<double>(value_))) : fallback; }
long long Json::asLong(long long fallback) const { return isNumber() ? std::llround(std::get<double>(value_)) : fallback; }
std::string Json::asString(const std::string& fallback) const { return isString() ? std::get<std::string>(value_) : fallback; }
const Json::Array& Json::asArray() const { if (!isArray()) throw std::runtime_error("JSON ist kein Array"); return std::get<Array>(value_); }
Json::Array& Json::asArray() { if (!isArray()) value_ = Array{}; return std::get<Array>(value_); }
const Json::Object& Json::asObject() const { if (!isObject()) throw std::runtime_error("JSON ist kein Objekt"); return std::get<Object>(value_); }
Json::Object& Json::asObject() { if (!isObject()) value_ = Object{}; return std::get<Object>(value_); }
const Json& Json::at(const std::string& key) const { if (!isObject()) return kNull; auto it = std::get<Object>(value_).find(key); return it == std::get<Object>(value_).end() ? kNull : it->second; }
Json& Json::at(const std::string& key) { return (*this)[key]; }
const Json& Json::operator[](const std::string& key) const { return at(key); }
Json& Json::operator[](const std::string& key) { return asObject()[key]; }
const Json& Json::operator[](std::size_t idx) const { if (!isArray() || idx >= std::get<Array>(value_).size()) return kNull; return std::get<Array>(value_)[idx]; }
Json& Json::operator[](std::size_t idx) { auto& a = asArray(); if (idx >= a.size()) a.resize(idx + 1); return a[idx]; }
bool Json::contains(const std::string& key) const { return isObject() && std::get<Object>(value_).count(key) > 0; }
std::size_t Json::size() const { if (isArray()) return std::get<Array>(value_).size(); if (isObject()) return std::get<Object>(value_).size(); return 0; }

} // namespace gba
