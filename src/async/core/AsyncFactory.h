/**
@file   AsyncFactory.h
@brief  Some templates used to support the creation of an object factory
@author Tobias Blomberg / SM0SVX
@date   2020-07-21

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2020 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
\endverbatim
*/

#ifndef ASYNC_FACTORY_INCLUDED
#define ASYNC_FACTORY_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <map>
#include <string>
#include <sstream>
#include <cassert>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

namespace Async
{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

/**
@brief  Base class for an object factory
@tparam T     The baseclass
@tparam Args  Contructor arguments, if any
@author Tobias Blomberg / SM0SVX
@date   2020-07-21

This class implements the actual factory. The type of the factory class would
be something like Async::Factory<Animal> for a base class representing animals.
It is most often adviced to create a typedef to represent the type of the
factory, e.g.:

@code
  typedef Async::Factory<Animal> AnimalFactory;
@endcode

Creating a typedef bocomes incresingly convenient when the constructor of the
objects being manufactured take one or more arguments since the type definition
will become quite long.

Also the addition of a convenience function for creating objects make the usage
of the factory easier and more readable. This function make use of the typedef
declared above and also rely on that a convenience struct for the specific
factories have been declared as described for Async::SpecificFactory.

@code
  Animal* createAnimal(const std::string& obj_name)
  {
    static AnimalSpecificFactory<Dog> dog_factory;
    static AnimalSpecificFactory<Cat> cat_factory;
    static AnimalSpecificFactory<Fox> fox_factory;
    return AnimalFactory::createNamedObject(obj_name);
  }
@endcode

The following example will describe in more detail how to use the factory
classes.

@example AsyncFactory_demo.cpp
*/
template <class T, typename... Args>
class Factory
{
  public:
    /**
     * @brief   Create an instance of the named class
     * @param   name The name of the class to create an instance of
     * @param   args Arguments to pass to constructor, if any
     * @return  Returns a newly constucted object or 0 on failure
     */
    static T *createNamedObject(const std::string& name, Args... args)
    {
      typename std::map<std::string, Factory<T, Args...>*>::iterator it;
      it = factories().find(name);
      if (it == factories().end())
      {
        return 0;
      }

      return (*it).second->createObject(args...);
    } /* Factory::createNamedObject */

    /**
     * @brief   Get a list of valid class names
     * @return  Return a string containing a list of valid class names
     *
     * This function is mostly useful for printing a message informing about
     * which classes are available.
     */
    static std::string validFactories(void)
    {
      std::stringstream ss;
      for (typename FactoryMap::const_iterator it = factories().begin();
           it != factories().end(); ++it)
      {
        ss << "\"" << (*it).first << "\" ";
      }
      return ss.str();
    }

    /**
     * @brief   Constructor
     * @param   name The name of the specific object factory being created
     */
    Factory(const std::string &name) : m_name(name)
    {
      typename FactoryMap::iterator it = factories().find(m_name);
      assert(it == factories().end());
      factories()[name] = this;
    } /* Factory::Factory */

    /**
     * @brief   Don't allow copy construction
     */
    Factory(const Factory<T, Args...>&) = delete;

    /**
     * @brief   Don't allow assignment
     */
    Factory& operator=(const Factory<T, Args...>&) = delete;

    /**
     * @brief   Destructor
     */
    virtual ~Factory(void)
    {
      typename FactoryMap::iterator it = factories().find(m_name);
      assert(it != factories().end());
      factories().erase(it);
    } /* Factory::~Factory */

  protected:
    /**
     * @brief   Create and return a new instance of an object
     * @param   args Arguments to pass to constructor, if any
     * @return  Returns a newly created instance of the object
     */
    virtual T *createObject(Args... args) = 0;

  private:
    typedef std::map<std::string, Factory<T, Args...>*> FactoryMap;
    std::string m_name;

    static FactoryMap& factories(void)
    {
      static FactoryMap factory_map;
      return factory_map;
    }
};  /* class Factory */


/**
@brief  Base class for a specific object factory
@tparam Base  The baseclass
@tparam T     The specific class
@tparam Args  Contructor arguments, if any
@author Tobias Blomberg / SM0SVX
@date   2020-07-21

This class is used as the base for a specific factory. To make it easier to
declare and use new specific factories it is beneficial to declare the
convenience class below. In this example we assume that there is a base class,
Animal, that is inherited by specific animal classes.

@code
  template <class T>
  struct AnimalSpecificFactory : public Async::SpecificFactory<Animal, T>
  {
    AnimalSpecificFactory(void)
      : Async::SpecificFactory<Animal, T>(T::OBJNAME) {}
  };
@endcode

The struct above relies on the fact that each specific animal class have
declared a constant, OBJNAME. That constant specify the name for the class
that is used when using the factory to create objects, i.e.:

@code
  struct Dog : public Animal
  {
    static constexpr const char* OBJNAME = "dog";
    // ...
  };
@endcode

To make each specific class available to the factory an instance of each
specific factory will have to be created before calling the
Async::Factory::createNamedObject function, i.e.:

@code
  AnimalSpecificFactory<Dog> dog_factory;
@endcode

The instatiation of all specific factories is most easily placed in a function
that is declared in the same files as the base class, as described in the
documentation for the Async::Factory class.

The following example will describe in more detail how to use the factory
classes.

@example AsyncFactory_demo.cpp
*/
template <class Base, class T, typename... Args>
class SpecificFactory : public Factory<Base, Args...>
{
  public:
    /**
     * @brief   Constructor
     * @param   name The name of the specific object factory being created
     */
    SpecificFactory(const std::string &name) : Factory<Base, Args...>(name) {}

  protected:
    /**
     * @brief   Create and return a new instance of an object
     * @return  Returns a newly created instance of the object
     */
    T *createObject(Args... args)
    {
      return new T(args...);
    }
};


} /* namespace Async */

#endif /* ASYNC_FACTORY_INCLUDED */

/*
 * This file has not been truncated
 */
