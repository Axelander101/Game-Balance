#pragma once
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace gba {

class Json {
public:
    using Array = std::vector<Json>;
    using Object = std::map<std::string, Json>;
    using Value = std::variant<std::nullptr_t, bool, double, std::string, Array, Object>;

    Json();
    Json(std::nullptr_t);
    Json(bool v);
    Json(int v);
    Json(long long v);
    Json(double v);
    Json(const char* v);
    Json(std::string v);
    Json(Array v);
    Json(Object v);

    static Json parse(const std::string& text);
    static Json parseFile(const std::string& path);
    void writeFile(const std::string& path, int indent = 2) const;
    std::string dump(int indent = 0) const;

    bool isNull() const;
    bool isBool() const;
    bool isNumber() const;
    bool isString() const;
    bool isArray() const;
    bool isObject() const;

    bool asBool(bool fallback = false) const;
    double asNumber(double fallback = 0.0) const;
    int asInt(int fallback = 0) const;
    long long asLong(long long fallback = 0) const;
    std::string asString(const std::string& fallback = "") const;

    const Array& asArray() const;
    Array& asArray();
    const Object& asObject() const;
    Object& asObject();

    const Json& at(const std::string& key) const;
    Json& at(const std::string& key);
    const Json& operator[](const std::string& key) const;
    Json& operator[](const std::string& key);
    const Json& operator[](std::size_t idx) const;
    Json& operator[](std::size_t idx);

    bool contains(const std::string& key) const;
    std::size_t size() const;

private:
    Value value_;
};

} // namespace gba
