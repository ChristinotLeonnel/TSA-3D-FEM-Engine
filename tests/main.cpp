#include "TestFramework.h"

#include <exception>

int main()
{
    int failed = 0;
    for (const auto& test : Fem3DTest::registry())
    {
        try
        {
            test.body();
            std::cout << "[PASS] " << test.name << '\n';
        }
        catch (const Fem3DTest::Failure& f)
        {
            ++failed;
            std::cout << "[FAIL] " << test.name << " : " << f.message << '\n';
        }
        catch (const std::exception& e)
        {
            ++failed;
            std::cout << "[FAIL] " << test.name << " : exception " << e.what() << '\n';
        }
    }
    std::cout << "RESULTATS : " << Fem3DTest::registry().size() - failed << " / " << Fem3DTest::registry().size() << " tests reussis\n";
    return failed;
}
