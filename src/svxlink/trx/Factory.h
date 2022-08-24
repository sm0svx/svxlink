/**
@file	 Factory.h
@brief   Some templates used to support the creation of an object factory class
@author  Tobias Blomberg / SM0SVX
@date	 2014-01-26

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2014 Tobias Blomberg / SM0SVX

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

#ifndef FACTORY_INCLUDED
#define FACTORY_INCLUDED


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
@brief	Base class for an object factory
@author Tobias Blomberg / SM0SVX
@date   2014-01-26

Use this class as a base when creating an object factory. For example, to
create a factory for a class called "MyObj", use the code below.

  struct MyObjFactoryBase : public FactoryBase<MyObj>
  {
    MyObjFactoryBase(const std::string &name) : FactoryBase<MyObj>(name) {}
  };

*/
template <class T>
class FactoryBase
{
  public:
    /**
     * @brief 	Create an instance of the named class
     * @param 	name The name of the class to create an instance of
     * @return	Returns a newly constucted object or 0 on failure
     */
    static T *createNamedObject(const std::string& name)
    {
      typename std::map<std::string, FactoryBase<T>*>::iterator it;
      it = factories.find(name);
      if (it == factories.end())
      {
        return 0;
      }

      return (*it).second->createObject();
    } /* Factory::createNamedObject */

    /**
     * @brief 	Get a list of valid class names
     * @return	Return a string containing a list of valid class names
     */
    static std::string validFactories(void)
    {
      std::stringstream ss;
      for (typename FactoryMap::const_iterator it = factories.begin();
           it != factories.end(); ++it)
      {
        ss << "\"" << (*it).first << "\" ";
      }
      return ss.str();
    }
    
    /**
     * @brief 	Constructor
     * @param   name The name of the specific object factory being created
     */
    FactoryBase(const std::string &name)
      : m_name(name)
    {
      typename FactoryMap::iterator it = factories.find(m_name);
      assert(it == factories.end());
      factories[name] = this;
    } /* Factory::Factory */

    /**
     * @brief 	Destructor
     */
    virtual ~FactoryBase(void)
    {
      typename FactoryMap::iterator it = factories.find(m_name);
      assert(it != factories.end());
      factories.erase(it);
    } /* Factory::~Factory */
    
  protected:
    /**
     * @brief 	Create and return a new instance of an object
     * @return	Returns a newly created instance of the object
     */
    virtual T *createObject(void) = 0;
  
  private:
    typedef std::map<std::string, FactoryBase<T>*> FactoryMap;
    static FactoryMap factories;
    std::string m_name;

    FactoryBase(const FactoryBase<T>&);
    FactoryBase& operator=(const FactoryBase<T>&);
    
};  /* class FactoryBase */


template <class T>
typename FactoryBase<T>::FactoryMap FactoryBase<T>::factories;


/**
@brief	Base class for a specific object factory
@author Tobias Blomberg / SM0SVX
@date   2014-01-26

This class should be used as the base for a specific factory. Let's say we
have a couple of classes MyObjOne and MyObjTwo which use MyObj as a base class.
Declare the following two classes to make it possible to create any of the two
objects using a factory.

  struct MyObjOneFactory : public MyObjFactory<MyObjOne>
  {
    MyObjOneFactory(void) : MyObjFactory<MyObjOne>("One") {}
  };
  struct MyObjTwoFactory : public MyObjFactory<MyObjTwo>
  {
    MyObjTwoFactory(void) : MyObjFactory<MyObjTwo>("Two") {}
  };

Now you need to instantiate one instance of each class so that they will
register in the factory. This have to be done before calling the
createNamedObject function.
*/
template <class FactoryT, class T>
class Factory : public FactoryT
{
  public:
    /**
     * @brief 	Constructor
     * @param   name The name of the specific object factory being created
     */
    Factory(const std::string &name) : FactoryT(name) {}

  protected:
    /**
     * @brief 	Create and return a new instance of an object
     * @return	Returns a newly created instance of the object
     */
    T *createObject(void)
    {
      return new T;
    }
};


//} /* namespace */

#endif /* FACTORY_INCLUDED */



/*
 * This file has not been truncated
 */
