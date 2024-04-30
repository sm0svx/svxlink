//
// This example demonstrates how to use the Async::Factory classes to enable a
// class being created using the factory pattern.
//
// The scenario is that we have a base class, Animal, which specific animal
// classes derive from, e.g. Dog, Cat etc. Each animal have a given name which
// is stored in the base class. That name is given to the constructor of each
// animal which demonstrates how to use the Async::Factory class with
// constructors that take one or more arguments. This is made possible by the
// fact that the Async::Factory classes use variadic templates.
//
// The base class also have a pure virtual function, "say", that each derived
// class must implement. It should return a string describing what the specific
// animal say.
//
// Just before main() a number of convenience constructs; a typedef, a
// templated struct and a function, have been added to make using the
// Async::Factory easier. These would normally go into the same files as where
// the base class is declared.
//

#include <iostream>
#include <memory>
#include <AsyncFactory.h>


  // The base class for all animals. This class has no connection to the
  // Async::Factory classes.
class Animal
{
  public:
    Animal(const std::string& given_name) : m_given_name(given_name) {}
    virtual ~Animal(void) {}
    const std::string& givenName(void) const { return m_given_name; }
    virtual const char* say(void) const = 0;
  private:
    std::string m_given_name;
};


  // A class representing a dog. The OBJNAME constant is needed to make the
  // convenience classas defined below work.
struct Dog : public Animal
{
  static constexpr auto OBJNAME = "dog";
  Dog(const std::string& given_name) : Animal(given_name) {}
  virtual const char* say(void) const override { return "Voff"; }
};


  // A class representing a cat. The OBJNAME constant is needed to make the
  // convenience classas defined below work.
struct Cat : public Animal
{
  static constexpr auto OBJNAME = "cat";
  Cat(const std::string& given_name) : Animal(given_name) {}
  virtual const char* say(void) const override { return "Meow"; }
};


  // A class representing a fox. The OBJNAME constant is needed to make the
  // convenience classas defined below work.
struct Fox : public Animal
{
  static constexpr auto OBJNAME = "fox";
  Fox(const std::string& given_name) : Animal(given_name) {}
  virtual const char* say(void) const override
  {
    return "Ding, ding, ding, ding, ding, di-ding, di-ding";
  }
};


  // Add a custom animal, in this case a cow. This animal is not part of the
  // "core animals" but instead it is added just before calling createAnimal.
struct Cow : public Animal
{
  static constexpr auto OBJNAME = "cow";
  Cow(const std::string& given_name) : Animal(given_name) {}
  virtual const char* say(void) const override
  {
    return "Moooo";
  }
};


  // A convenience struct to make instantiation of spcecific animals easier.
  // This class relies on the fact that each specific animal class have
  // declared a constant, OBJNAME. That constant specify the name for the class
  // that is used when using the factory to create objects.
template <class T>
struct AnimalSpecificFactory
  : public Async::SpecificFactory<Animal, T, std::string>
{
  AnimalSpecificFactory(void)
    : Async::SpecificFactory<Animal, T, std::string>(T::OBJNAME) {}
};

  // A convenience typedef to make access to Async::Factory members easier.
using AnimalFactory = Async::Factory<Animal, std::string>;

  // A function for creating an animal
std::unique_ptr<Animal> createAnimal(const std::string& obj_name,
                                     const std::string& animal_name)
{
  static AnimalSpecificFactory<Dog> dog_factory;
  static AnimalSpecificFactory<Cat> cat_factory;
  static AnimalSpecificFactory<Fox> fox_factory;
  return std::unique_ptr<Animal>(
      AnimalFactory::createNamedObject(obj_name, animal_name));
}


int main(int argc, const char *argv[])
{
  if (argc < 3)
  {
    std::cout << "Usage: " << argv[0] << " <obj name> <given name>"
              << std::endl;
    return 1;
  }
  const std::string obj_name(argv[1]);
  const std::string given_name(argv[2]);

    // Add our custom animal before calling createAnimal
  AnimalSpecificFactory<Cow> cow_factory;

    // Create the animal specified on the command line
  auto obj = createAnimal(obj_name, given_name);
  if (obj == nullptr)
  {
    std::cout << "Sorry, but there is no animal \"" << argv[1] << "\" :-("
              << std::endl;
    std::cout << "Valid animals: " << AnimalFactory::validFactories()
              << std::endl;
    exit(1);
  }
  std::cout << "- Hello, " << obj->givenName() << "! What do you say?\n- "
            << obj->say() << "!" << std::endl;
}
