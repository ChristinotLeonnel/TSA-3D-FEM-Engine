#pragma once

// Mini-cadre de test sans dépendance : FEM3D_TEST(nom) { … } ; CHECK(cond, message) ; CHECK_NEAR(a, b, tol, message).
// Un échec affiche le fichier, la ligne, les valeurs et poursuit les autres tests ; le code de sortie vaut le nombre
// de tests en échec.

#include <cmath>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace Fem3DTest
{

struct TestCase
{
    std::string name;
    std::function<void()> body;
};

inline std::vector<TestCase>& registry()
{
    static std::vector<TestCase> tests;
    return tests;
}

struct Failure
{
    std::string message;
};

struct Registrar
{
    Registrar(const char* name, std::function<void()> body) { registry().push_back({ name, std::move(body) }); }
};

/// Écart admissible : |a − b| ≤ max(absolute, relative · |b|) (critère absolu près de zéro).
inline bool near(double a, double b, double relative, double absolute = 1e-12)
{
    return std::abs(a - b) <= std::max(absolute, relative * std::abs(b));
}

} // namespace Fem3DTest

#define FEM3D_CONCAT2(a, b) a##b
#define FEM3D_CONCAT(a, b) FEM3D_CONCAT2(a, b)
#define FEM3D_TEST(name)                                                                                     \
    static void name();                                                                                      \
    static Fem3DTest::Registrar FEM3D_CONCAT(registrar_, name)(#name, name);                                 \
    static void name()

#define CHECK(cond, msg)                                                                                     \
    do                                                                                                       \
    {                                                                                                        \
        if (!(cond))                                                                                         \
        {                                                                                                    \
            std::ostringstream os_;                                                                          \
            os_ << __FILE__ << ":" << __LINE__ << " : " << msg << " (" #cond ")";                            \
            throw Fem3DTest::Failure{ os_.str() };                                                           \
        }                                                                                                    \
    } while (0)

#define CHECK_NEAR(a, b, rel, msg)                                                                           \
    do                                                                                                       \
    {                                                                                                        \
        const double a_ = (a), b_ = (b);                                                                     \
        if (!Fem3DTest::near(a_, b_, (rel)))                                                                 \
        {                                                                                                    \
            std::ostringstream os_;                                                                          \
            os_.precision(12);                                                                               \
            os_ << __FILE__ << ":" << __LINE__ << " : " << msg << " : obtenu " << a_ << ", attendu " << b_;  \
            throw Fem3DTest::Failure{ os_.str() };                                                           \
        }                                                                                                    \
    } while (0)
