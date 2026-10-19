#include "fem3dpost/Calculations.h"

#include <algorithm>
#include <cmath>

namespace fem3dpost {

std::optional<ChordDeflection> chordDeflection(const Element& element, const ElementResult& result,
                                               const Station& station)
{
    const auto& d = result.localDisplacements;
    const auto& s = station.localDisplacement;
    if (!d[1] || !d[2] || !d[7] || !d[8] || !s[1] || !s[2] || !(element.length > 0.0))
        return std::nullopt;
    const double t = station.x / element.length;
    ChordDeflection c;
    c.v = *s[1] - ((1.0 - t) * *d[1] + t * *d[7]);
    c.w = *s[2] - ((1.0 - t) * *d[2] + t * *d[8]);
    c.magnitude = std::hypot(c.v, c.w);
    return c;
}

OptDouble maxMeanShear(const Stresses& s)
{
    if (!s.shearY || !s.shearZ)
        return std::nullopt;
    return std::max(std::fabs(*s.shearY), std::fabs(*s.shearZ));
}

SeriesExtremes seriesExtremes(const std::vector<double>& x, const std::vector<OptDouble>& values)
{
    SeriesExtremes e;
    const std::size_t n = std::min(x.size(), values.size());
    for (std::size_t i = 0; i < n; ++i) {
        if (!values[i])
            continue;
        const double v = *values[i];
        if (!e.min || v < e.min->value)
            e.min = Extremum{v, x[i]};
        if (!e.max || v > e.max->value)
            e.max = Extremum{v, x[i]};
        if (!e.maxAbs || std::fabs(v) > std::fabs(e.maxAbs->value))
            e.maxAbs = Extremum{v, x[i]};
    }
    return e;
}

Resultant reactionResultant(const Results& results, const LoadCase& loadCase)
{
    std::array<double, 3> f{}, m{};
    bool complete = true;
    // Appuis rigides puis ressorts : le bloc « equilibrium » du moteur somme les deux.
    auto add = [&](int nodeId, const NodalValues* r) {
        const Node* node = results.node(nodeId);
        if (!r || !node) {
            complete = false;
            return;
        }
        for (const auto& v : r->values)
            if (!v)
                complete = false;
        if (!complete)
            return;
        const auto& p = node->xyz;
        const double fx = *r->values[0], fy = *r->values[1], fz = *r->values[2];
        f[0] += fx;
        f[1] += fy;
        f[2] += fz;
        m[0] += p[1] * fz - p[2] * fy + *r->values[3];
        m[1] += p[2] * fx - p[0] * fz + *r->values[4];
        m[2] += p[0] * fy - p[1] * fx + *r->values[5];
    };
    for (const Support& sup : results.supports)
        add(sup.node, loadCase.reactionOf(sup.node));
    for (const Spring& spring : results.springs)
        add(spring.node, loadCase.springForceOf(spring.node));
    Resultant out;
    if (complete) {
        for (int i = 0; i < 3; ++i) {
            out.force[static_cast<std::size_t>(i)] = f[static_cast<std::size_t>(i)];
            out.moment[static_cast<std::size_t>(i)] = m[static_cast<std::size_t>(i)];
        }
    }
    return out;
}

OptDouble normOfSum(const std::array<OptDouble, 3>& a, const std::array<OptDouble, 3>& b)
{
    double s = 0.0;
    for (std::size_t i = 0; i < 3; ++i) {
        if (!a[i] || !b[i])
            return std::nullopt;
        const double c = *a[i] + *b[i];
        s += c * c;
    }
    return std::sqrt(s);
}

OptDouble norm(const std::array<OptDouble, 3>& a)
{
    double s = 0.0;
    for (const auto& v : a) {
        if (!v)
            return std::nullopt;
        s += *v * *v;
    }
    return std::sqrt(s);
}

} // namespace fem3dpost
