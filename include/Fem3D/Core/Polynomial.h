#pragma once

// Polynôme p(x) = c0 + c1 x + c2 x² + … (abscisse locale x de l'élément, 0 ≤ x ≤ L).
// Les efforts internes sous charges réparties linéaires sont polynomiaux (N, V : degré ≤ 2 ; M : degré ≤ 3) :
// leurs intégrales (rotations, déplacements) sont calculées exactement, sans quadrature.
// Polynôme par morceaux : charges ponctuelles et charges sur une partie de la barre (efforts discontinus ou de
// dérivée discontinue aux abscisses des charges), un polynôme par intervalle sans discontinuité.

#include <algorithm>
#include <cstddef>
#include <vector>

namespace TSA::Fem3D
{

class Polynomial
{
public:
    Polynomial() = default;
    explicit Polynomial(std::vector<double> coefficients)
        : m_c(std::move(coefficients))
    {
    }
    static Polynomial constant(double c0) { return Polynomial({ c0 }); }
    static Polynomial linear(double c0, double c1) { return Polynomial({ c0, c1 }); }

    const std::vector<double>& coefficients() const { return m_c; }

    double operator()(double x) const
    {
        double y = 0.0;
        for (std::size_t k = m_c.size(); k-- > 0;) y = y * x + m_c[k];   // Horner
        return y;
    }

    /// Primitive nulle en x = 0 : ∫₀ˣ p(s) ds.
    Polynomial integral() const
    {
        std::vector<double> c(m_c.size() + 1, 0.0);
        for (std::size_t k = 0; k < m_c.size(); ++k) c[k + 1] = m_c[k] / static_cast<double>(k + 1);
        return Polynomial(std::move(c));
    }

    Polynomial derivative() const
    {
        if (m_c.size() <= 1) return Polynomial({ 0.0 });
        std::vector<double> c(m_c.size() - 1);
        for (std::size_t k = 1; k < m_c.size(); ++k) c[k - 1] = m_c[k] * static_cast<double>(k);
        return Polynomial(std::move(c));
    }

    Polynomial& operator+=(const Polynomial& o)
    {
        m_c.resize(std::max(m_c.size(), o.m_c.size()), 0.0);
        for (std::size_t k = 0; k < o.m_c.size(); ++k) m_c[k] += o.m_c[k];
        return *this;
    }
    Polynomial& operator*=(double s)
    {
        for (double& c : m_c) c *= s;
        return *this;
    }
    friend Polynomial operator+(Polynomial a, const Polynomial& b) { return a += b; }
    friend Polynomial operator-(Polynomial a, const Polynomial& b) { return a += (-1.0) * b; }
    friend Polynomial operator*(double s, Polynomial p) { return p *= s; }
    /// Produit par le monôme x.
    Polynomial timesX() const
    {
        std::vector<double> c(m_c.size() + 1, 0.0);
        for (std::size_t k = 0; k < m_c.size(); ++k) c[k + 1] = m_c[k];
        return Polynomial(std::move(c));
    }

private:
    std::vector<double> m_c;
};

/// Fonction polynomiale par morceaux sur [x₀, x_n] : sur [x_k, x_{k+1}[, p(x) = P_k(x − x_k) (variable locale du
/// morceau : coefficients bien conditionnés quelle que soit l'unité de longueur). Continue à droite : à une abscisse de
/// rupture x_k (charge ponctuelle), la valeur est celle du morceau qui commence en x_k ; le dernier morceau vaut
/// jusqu'à x_n compris.
class PiecewisePolynomial
{
public:
    PiecewisePolynomial() = default;
    /// starts : débuts des morceaux, croissants ; pieces : un polynôme (en x − début) par morceau.
    PiecewisePolynomial(std::vector<double> starts, std::vector<Polynomial> pieces)
        : m_starts(std::move(starts))
        , m_pieces(std::move(pieces))
    {
    }

    const std::vector<double>& starts() const { return m_starts; }
    const std::vector<Polynomial>& pieces() const { return m_pieces; }

    double operator()(double x) const
    {
        if (m_pieces.empty()) return 0.0;
        const auto it = std::upper_bound(m_starts.begin(), m_starts.end(), x);
        const std::size_t k = it == m_starts.begin() ? 0 : std::min<std::size_t>(static_cast<std::size_t>(it - m_starts.begin()) - 1, m_pieces.size() - 1);
        return m_pieces[k](x - m_starts[k]);
    }

private:
    std::vector<double> m_starts;
    std::vector<Polynomial> m_pieces;
};

} // namespace TSA::Fem3D
