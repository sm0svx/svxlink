# ModulationTest compiles the small Modulation.cpp source directly (it tests
# Modulation::fromString/toString). The trx static library also contains
# Modulation.o, but since this object resolves the symbols directly the
# archive member is not pulled in, so there is no duplicate-symbol conflict.
set(ModulationTest_EXTRA_SRCS Modulation.cpp)
