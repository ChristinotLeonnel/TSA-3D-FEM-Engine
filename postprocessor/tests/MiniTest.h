#pragma once
// Mini-cadre de test maison (aucune dépendance externe).
//   TEST_CASE("nom") { CHECK(expr); CHECK_EQ(a, b); REQUIRE(expr); CHECK_THROWS(expr, Type); }

#include <cstdio>
#include <exception>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

namespace minitest {

struct Test {
    const char* name;
    std::function<void()> body;
};

inline std::vector<Test>& registry()
{
    static std::vector<Test> tests;
    return tests;
}

struct State {
    int checks = 0;
    int failures = 0;
};

inline State& state()
{
    static State s;
    return s;
}

struct Registrar {
    Registrar(const char* name, std::function<void()> body) { registry().push_back({name, std::move(body)}); }
};

// Échec d'un REQUIRE : interrompt le test courant.
struct RequireFailure {};

inline void report(bool ok, const char* expr, const char* file, int line, const std::string& detail = {})
{
    ++state().checks;
    if (ok)
        return;
    ++state().failures;
    std::printf("    ÉCHEC %s:%d : %s%s%s\n", file, line, expr, detail.empty() ? "" : " — ", detail.c_str());
}

template <class A, class B>
std::string describe(const A& a, const B& b)
{
    std::ostringstream o;
    o.precision(17);
    o << "obtenu « " << a << " », attendu « " << b << " »";
    return o.str();
}

inline int runAll()
{
    int failedTests = 0;
    for (const Test& t : registry()) {
        const int before = state().failures;
        std::printf("[ TEST ] %s\n", t.name);
        try {
            t.body();
        } catch (const RequireFailure&) {
            // déjà signalé
        } catch (const std::exception& e) {
            ++state().failures;
            std::printf("    ÉCHEC : exception inattendue : %s\n", e.what());
        } catch (...) {
            ++state().failures;
            std::printf("    ÉCHEC : exception inconnue\n");
        }
        const bool ok = state().failures == before;
        if (!ok)
            ++failedTests;
        std::printf("[ %s ] %s\n", ok ? " OK " : "ÉCHEC", t.name);
    }
    std::printf("\n%zu tests, %d vérifications, %d test(s) en échec, %d vérification(s) en échec\n",
                registry().size(), state().checks, failedTests, state().failures);
    return failedTests == 0 ? 0 : 1;
}

} // namespace minitest

#define MT_CAT2(a, b) a##b
#define MT_CAT(a, b) MT_CAT2(a, b)
#define TEST_CASE(name)                                                                                     \
    static void MT_CAT(mt_test_, __LINE__)();                                                               \
    static const minitest::Registrar MT_CAT(mt_reg_, __LINE__)(name, &MT_CAT(mt_test_, __LINE__));          \
    static void MT_CAT(mt_test_, __LINE__)()

#define CHECK(expr) minitest::report(static_cast<bool>(expr), #expr, __FILE__, __LINE__)
#define CHECK_EQ(a, b)                                                                                      \
    do {                                                                                                    \
        const auto& mt_a = (a);                                                                             \
        const auto& mt_b = (b);                                                                             \
        const bool mt_ok = (mt_a == mt_b);                                                                  \
        minitest::report(mt_ok, #a " == " #b, __FILE__, __LINE__, mt_ok ? std::string() : minitest::describe(mt_a, mt_b)); \
    } while (0)
#define REQUIRE(expr)                                                                                       \
    do {                                                                                                    \
        const bool mt_ok = static_cast<bool>(expr);                                                         \
        minitest::report(mt_ok, #expr, __FILE__, __LINE__);                                                 \
        if (!mt_ok)                                                                                         \
            throw minitest::RequireFailure{};                                                               \
    } while (0)
#define CHECK_THROWS(expr, Type)                                                                            \
    do {                                                                                                    \
        bool mt_thrown = false;                                                                             \
        try {                                                                                               \
            (void)(expr);                                                                                   \
        } catch (const Type&) {                                                                             \
            mt_thrown = true;                                                                               \
        } catch (...) {                                                                                     \
        }                                                                                                   \
        minitest::report(mt_thrown, #expr " lève " #Type, __FILE__, __LINE__);                              \
    } while (0)
