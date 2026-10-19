// Tests unitaires : repère local, matrices élémentaires, polynômes, contrôles d'entrée.

#include "TestFramework.h"
#include "TestModels.h"

#include "Fem3D/Core/Polynomial.h"
#include "Fem3D/Elements/EulerBernoulli3D.h"
#include "Fem3D/Elements/Timoshenko3D.h"

#include <Eigen/Eigenvalues>

using namespace Fem3DTest;

namespace
{
Matrix12 rigidBodyModes(const Vector3& start, const Vector3& end)
{
    // Colonnes : 3 translations, 3 rotations infinitésimales autour de l'origine (u = θ × r), repère global
    Matrix12 modes = Matrix12::Zero();
    for (int k = 0; k < 3; ++k)
    {
        modes(k, k) = 1.0;
        modes(6 + k, k) = 1.0;
        const Vector3 axis = Vector3::Unit(k);
        const Vector3 ui = axis.cross(start), uj = axis.cross(end);
        modes.block<3, 1>(0, 3 + k) = ui;
        modes.block<3, 1>(6, 3 + k) = uj;
        modes(3 + k, 3 + k) = 1.0;
        modes(9 + k, 3 + k) = 1.0;
    }
    return modes;
}

template <typename Element> Element makeElement(const Vector3& a, const Vector3& b, BeamSection3D section = rectangle())
{
    return Element(1, 1, 2, LocalCoordinateSystem3D(a, b), steel(), section);
}
} // namespace

FEM3D_TEST(LocalAxes_HorizontalVerticalInclined)
{
    const LocalCoordinateSystem3D h(Vector3::Zero(), Vector3(5, 0, 0));
    CHECK(h.xAxis().isApprox(Vector3::UnitX()) && h.yAxis().isApprox(Vector3::UnitY()) && h.zAxis().isApprox(Vector3::UnitZ()),
          "poutre selon X : y = Y, z = Z");
    const LocalCoordinateSystem3D v(Vector3::Zero(), Vector3(0, 0, 3));
    CHECK(v.yAxis().isApprox(Vector3::UnitY()) && v.zAxis().isApprox(-Vector3::UnitX()), "poteau selon +Z : y = Y, z = −X");
    const LocalCoordinateSystem3D i(Vector3(1, 2, 3), Vector3(4, 6, 15));
    CHECK_NEAR(i.length(), 13.0, 1e-14, "longueur réelle");
    const Matrix3 r = i.rotation();
    CHECK((r * r.transpose()).isApprox(Matrix3::Identity(), 1e-14) && std::abs(r.determinant() - 1.0) < 1e-14, "base orthonormée directe");
    const LocalCoordinateSystem3D o(Vector3::Zero(), Vector3(5, 0, 0), Vector3(0, 1, 1));
    CHECK(std::abs(o.zAxis().dot(Vector3(0, 1, 1).normalized()) - 1.0) < 1e-14, "v_xz fourni : z dans le plan (x, v_xz), orienté comme v_xz");
}

FEM3D_TEST(LocalAxes_DegenerateGeometryRejected)
{
    bool zeroLength = false, parallel = false;
    try { LocalCoordinateSystem3D(Vector3(1, 1, 1), Vector3(1, 1, 1)); }
    catch (const Fem3DError& e) { zeroLength = e.code() == ErrorCode::DegenerateGeometry; }
    try { LocalCoordinateSystem3D(Vector3::Zero(), Vector3(2, 0, 0), Vector3(1, 0, 0)); }
    catch (const Fem3DError& e) { parallel = e.code() == ErrorCode::DegenerateGeometry; }
    CHECK(zeroLength && parallel, "longueur nulle et v_xz parallèle refusés");
}

FEM3D_TEST(Element_SymmetryAndRigidBodyModes)
{
    const Vector3 a(1, -2, 0.5), b(4, 2, 3.5);
    const EulerBernoulli3D eb = makeElement<EulerBernoulli3D>(a, b);
    const Timoshenko3D ti = makeElement<Timoshenko3D>(a, b);
    for (const IBeamElement3D* e : { static_cast<const IBeamElement3D*>(&eb), static_cast<const IBeamElement3D*>(&ti) })
    {
        const Matrix12 k = e->globalStiffnessMatrix();
        CHECK((k - k.transpose()).norm() <= 1e-12 * k.norm(), "matrice globale symétrique");
        const Matrix12 forces = k * rigidBodyModes(a, b);
        CHECK(forces.norm() <= 1e-9 * k.norm(), "aucun effort pour les 6 modes de corps rigide (" << formulationName(e->formulation()) << ")");
        Eigen::SelfAdjointEigenSolver<Matrix12> eig(k);
        int zero = 0;
        for (int i = 0; i < 12; ++i) zero += std::abs(eig.eigenvalues()(i)) < 1e-9 * eig.eigenvalues().cwiseAbs().maxCoeff();
        CHECK(zero == 6 && eig.eigenvalues().minCoeff() > -1e-6 * eig.eigenvalues().maxCoeff(),
              "exactement 6 valeurs propres nulles, aucune négative : pas de mécanisme artificiel");
    }
}

FEM3D_TEST(Element_TimoshenkoTendsToEulerBernoulli)
{
    BeamSection3D stiff = rectangle();
    stiff.shearAreaY = stiff.shearAreaZ = stiff.area * 1e9;   // cisaillement quasi rigide
    const Timoshenko3D ti = makeElement<Timoshenko3D>(Vector3::Zero(), Vector3(4, 0, 0), stiff);
    const EulerBernoulli3D eb = makeElement<EulerBernoulli3D>(Vector3::Zero(), Vector3(4, 0, 0), stiff);
    CHECK((ti.localStiffnessMatrix() - eb.localStiffnessMatrix()).norm() <= 1e-7 * eb.localStiffnessMatrix().norm(), "Φ → 0 : même rigidité");
    const auto nt = ti.shapeFunctions(0.37), ne = eb.shapeFunctions(0.37);
    for (int k = 0; k < 4; ++k) CHECK_NEAR(nt.bendingXY[static_cast<std::size_t>(k)], ne.bendingXY[static_cast<std::size_t>(k)], 1e-7, "fonctions de forme");
}

FEM3D_TEST(Element_ExplicitStiffnessTerms)
{
    const BeamSection3D s = rectangle();
    const double l = 4.0;
    const EulerBernoulli3D eb = makeElement<EulerBernoulli3D>(Vector3::Zero(), Vector3(l, 0, 0));
    const Matrix12 k = eb.localStiffnessMatrix();
    CHECK_NEAR(k(0, 0), kE * s.area / l, 1e-14, "EA/L");
    CHECK_NEAR(k(3, 3), kG * s.torsionConstant / l, 1e-14, "GJ/L");
    CHECK_NEAR(k(1, 1), 12 * kE * s.inertiaZ / (l * l * l), 1e-14, "12 E Iz / L³ (plan x-y)");
    CHECK_NEAR(k(2, 2), 12 * kE * s.inertiaY / (l * l * l), 1e-14, "12 E Iy / L³ (plan x-z)");
    CHECK_NEAR(k(1, 5), 6 * kE * s.inertiaZ / (l * l), 1e-14, "v-θz : +6 E Iz / L²");
    CHECK_NEAR(k(2, 4), -6 * kE * s.inertiaY / (l * l), 1e-14, "w-θy : −6 E Iy / L² (θy = −dw/dx)");
    const Timoshenko3D ti = makeElement<Timoshenko3D>(Vector3::Zero(), Vector3(l, 0, 0));
    const double phi = 12 * kE * s.inertiaZ / (kG * *s.shearAreaY * l * l);
    CHECK_NEAR(ti.shearParameterY(), phi, 1e-14, "Φy = 12 E Iz / (G Asy L²)");
    CHECK_NEAR(ti.localStiffnessMatrix()(5, 5), (4 + phi) * kE * s.inertiaZ / ((1 + phi) * l), 1e-13, "(4+Φ) E Iz / ((1+Φ) L)");
}

FEM3D_TEST(Polynomial_EvaluateIntegrate)
{
    const Polynomial p({ 1.0, -2.0, 3.0 });   // 1 − 2x + 3x²
    CHECK_NEAR(p(2.0), 9.0, 1e-15, "évaluation");
    CHECK_NEAR(p.integral()(2.0), 2.0 - 4.0 + 8.0, 1e-15, "primitive nulle en 0");
    CHECK_NEAR(p.derivative()(2.0), 10.0, 1e-15, "dérivée");
}

FEM3D_TEST(Input_InvalidPropertiesRejected)
{
    int rejected = 0;
    try { ElasticMaterial{ "x", -1.0, 1.0 }.validate(); } catch (const Fem3DError&) { ++rejected; }
    BeamSection3D s = rectangle();
    s.torsionConstant = 0.0;
    try { s.validate(false); } catch (const Fem3DError&) { ++rejected; }
    BeamSection3D noShear = rectangle();
    noShear.shearAreaY.reset();
    try { Timoshenko3D(1, 1, 2, LocalCoordinateSystem3D(Vector3::Zero(), Vector3(1, 0, 0)), steel(), noShear); } catch (const Fem3DError&) { ++rejected; }
    StructuralModel3D m = cantilever(2.0, BeamFormulation::EulerBernoulli);
    try { m.addNode(2, Vector3(9, 9, 9)); } catch (const Fem3DError& e) { rejected += e.code() == ErrorCode::DuplicateId; }
    CHECK(rejected == 4, "E ≤ 0, J nul, Timoshenko sans aire de cisaillement, identifiant en double : refusés");
}
