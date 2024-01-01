/**
@file    SquelchCombine.cpp
@brief   A squelch type that combines multiple squelch types
@author  Tobias Blomberg / SM0SVX
@date    2020-07-26

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2021 Tobias Blomberg / SM0SVX

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

/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>
#include <iterator>
#include <set>


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

#include "SquelchCombine.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;


/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/

class SquelchCombine::Node
{
  public:
    typedef std::set<std::string> SquelchStates;
    Node(const std::string& name) : m_name(name) {}
    virtual ~Node(void) {}
    const std::string name(void) const { return m_name; }
    virtual void print(std::ostream& os) = 0;
    virtual bool initialize(Async::Config& cfg) = 0;
    virtual void reset(void) = 0;
    virtual void restart(void) = 0;
    virtual void processSamples(const float *samples, int count) = 0;
    virtual bool isOpen(void) const = 0;
    virtual std::string activityInfo(void) const = 0;
    virtual SquelchStates& squelchStates(SquelchStates& states) = 0;
    sigc::signal<void, bool> squelchOpen;
    sigc::signal<void, float> toneDetected;

  private:
    std::string m_name;
}; /* SquelchCombine::Node */


class SquelchCombine::LeafNode : public SquelchCombine::Node
{
  public:
    LeafNode(const std::string n) : Node(n) {}

    virtual ~LeafNode(void)
    {
      delete m_squelch;
      m_squelch = nullptr;
    }

    virtual void print(std::ostream& os) { os << name(); }

    virtual bool initialize(Async::Config& cfg)
    {
      string sql_det_str;
      if (!cfg.getValue(name(), "SQL_DET", sql_det_str))
      {
        cerr << "*** ERROR: Config variable " << name() << "/SQL_DET not set\n";
        return false;
      }
      m_squelch = createSquelch(sql_det_str);
      if ((m_squelch == nullptr) || !m_squelch->initialize(cfg, name()))
      {
        std::cerr << "*** ERROR: Squelch detector initialization failed for \""
                  << name() << "\"\n";
        return false;
      }
      m_squelch->squelchOpen.connect(squelchOpen.make_slot());
      m_squelch->toneDetected.connect(toneDetected.make_slot());
      return true;
    }

    virtual bool isOpen(void) const { return m_squelch->isOpen(); }

    virtual std::string activityInfo(void) const
    {
      std::string act_info = name();
      if (m_squelch->isOpen())
      {
        act_info += "*";
      }
      if (!m_squelch->activityInfo().empty())
      {
        if (!m_squelch->isOpen())
        {
          act_info += "=";
        }
        act_info += m_squelch->activityInfo();
      }
      return act_info;
    }

    virtual void reset(void) { m_squelch->reset(); }

    virtual void restart(void) { m_squelch->restart(); }

    virtual void processSamples(const float *samples, int count)
    {
      int pos = 0;
      do {
        int ret = m_squelch->writeSamples(samples, count);
        if (ret < 1)
        {
          std::cout << "*** WARNING: Failed to write samples to squelch "
                       "detector \"" << name() << "\" in squelch combiner."
                    << std::endl;
          break;
        }
        pos += ret;
      } while (pos < count);
    }

    virtual SquelchStates& squelchStates(SquelchStates& states)
    {
      states.emplace(activityInfo());
      return states;
    }

  private:
    Squelch* m_squelch = nullptr;
}; /* SquelchCombine::LeafNode */


class SquelchCombine::UnaryOpNode : public SquelchCombine::Node
{
  public:
    UnaryOpNode(const std::string& name, Node *node)
      : Node(name), m_node(node)
    {
      m_node->squelchOpen.connect(squelchOpen.make_slot());
      m_node->toneDetected.connect(toneDetected.make_slot());
    }

    virtual ~UnaryOpNode(void)
    {
      delete m_node;
      m_node = nullptr;
    }

    virtual void print(std::ostream& os)
    {
      os << name() << "(";
      m_node->print(os);
      os << ")";
    }

    virtual bool initialize(Async::Config& cfg)
    {
      return m_node->initialize(cfg);
    }

    virtual void reset(void)
    {
      m_node->reset();
    }

    virtual void restart(void)
    {
      m_node->restart();
    }

    virtual void processSamples(const float *samples, int count)
    {
      m_node->processSamples(samples, count);
    }

    virtual SquelchStates& squelchStates(SquelchStates& states)
    {
      return m_node->squelchStates(states);
    }

    virtual std::string activityInfo(void) const
    {
      return name() + "(" + m_node->activityInfo() + ")";
    }

  protected:
    Node* m_node;
}; /* SquelchCombine::UnaryOpNode */


struct SquelchCombine::NegationOpNode : public SquelchCombine::UnaryOpNode
{
  NegationOpNode(Node* node) : UnaryOpNode("NOT", node) {}
  virtual bool isOpen(void) const { return !m_node->isOpen(); }
}; /* SquelchCombine::NegationOpNode */


class SquelchCombine::BinaryOpNode : public SquelchCombine::Node
{
  public:
    BinaryOpNode(const std::string& n, Node* l, Node* r)
      : Node(n), m_left(l), m_right(r)
    {
      m_left->squelchOpen.connect(
          sigc::mem_fun(*this, &BinaryOpNode::onSquelchOpen));
      m_left->toneDetected.connect(toneDetected.make_slot());
      m_right->squelchOpen.connect(
          sigc::mem_fun(*this, &BinaryOpNode::onSquelchOpen));
      m_right->toneDetected.connect(toneDetected.make_slot());
    }

    virtual ~BinaryOpNode(void)
    {
      delete m_left;
      m_left = nullptr;
      delete m_right;
      m_right = nullptr;
    }

    virtual void print(std::ostream& os)
    {
      os << name() << "(";
      m_left->print(os);
      os << ", ";
      m_right->print(os);
      os << ")";
    }

    virtual bool initialize(Async::Config& cfg)
    {
      return m_left->initialize(cfg) && m_right->initialize(cfg);
    }

    virtual void reset(void)
    {
      m_left->reset();
      m_right->reset();
    }

    virtual void restart(void)
    {
      m_left->restart();
      m_right->restart();
    }

    virtual void processSamples(const float *samples, int count)
    {
      m_left->processSamples(samples, count);
      m_right->processSamples(samples, count);
    }

    void onSquelchOpen(bool is_open)
    {
      squelchOpen(isOpen());
    }

    virtual SquelchStates& squelchStates(SquelchStates& states)
    {
      m_left->squelchStates(states);
      m_right->squelchStates(states);
      return states;
    }

    virtual std::string activityInfo(void) const
    {
      return name() + "(" + m_left->activityInfo() + ", " +
             m_right->activityInfo() + ")";
    }

  protected:
    Node* m_left;
    Node* m_right;
}; /* SquelchCombine::BinaryOpNode */


struct SquelchCombine::OrOpNode : public SquelchCombine::BinaryOpNode
{
  OrOpNode(Node* l, Node* r) : BinaryOpNode("OR", l, r) {}

  virtual bool isOpen(void) const
  {
    return m_left->isOpen() || m_right->isOpen();
  }
}; /* SquelchCombine::OrOpNode */


struct SquelchCombine::AndOpNode : public SquelchCombine::BinaryOpNode
{
  AndOpNode(Node* l, Node* r) : BinaryOpNode("AND", l, r) {}

  virtual bool isOpen(void) const
  {
    return m_left->isOpen() && m_right->isOpen();
  }
}; /* SquelchCombine::AndOpNode */


/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/




/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

SquelchCombine::SquelchCombine(void)
{
} /* SquelchCombine::SquelchCombine */


SquelchCombine::~SquelchCombine(void)
{
  delete m_comb;
} /* SquelchCombine::~SquelchCombine */


bool SquelchCombine::initialize(Async::Config& cfg,
                                const std::string& rx_name)
{
  string expr_str;
  if (!cfg.getValue(rx_name, "SQL_COMBINE", expr_str))
  {
    cerr << "*** ERROR: Could not read configuration variable "
         << rx_name << "/SQL_COMBINE" << endl;
    return false;
  }

  if (!tokenize(expr_str))
  {
    return false;
  }

  m_comb = parseExpression();
  if (!m_tokens.empty())
  {
    std::cout << "*** ERROR: Unparsed extra tokens in malformed squelch "
                 "combiner expression: ";
    copy(m_tokens.begin(), m_tokens.end(),
        std::ostream_iterator<std::string>(std::cout, " "));
    std::cout << std::endl;
    delete m_comb;
    m_comb = nullptr;
  }
  if (m_comb == nullptr)
  {
    std::cout << "*** ERROR: Failed to create combined squelch for RX \""
              << rx_name << "\"" << std::endl;
    return false;
  }

  std::cout << rx_name << ": Combined squelch structure is ";
  m_comb->print(std::cout);
  std::cout << std::endl;

  m_comb->squelchOpen.connect(
      sigc::mem_fun(*this, &SquelchCombine::onSquelchOpen));
  m_comb->toneDetected.connect(toneDetected.make_slot());

  return m_comb->initialize(cfg) && Squelch::initialize(cfg, rx_name);
} /* SquelchCombine::initialize */


void SquelchCombine::reset(void)
{
  m_comb->reset();
  Squelch::reset();
} /* SquelchCombine::reset */


void SquelchCombine::restart(void)
{
  m_comb->restart();
  Squelch::restart();
} /* SquelchCombine::restart */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

int SquelchCombine::processSamples(const float *samples, int count)
{
  m_comb->processSamples(samples, count);
  return count;
} /* SquelchCombine::processSamples */


/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void SquelchCombine::onSquelchOpen(bool is_open)
{
  if (is_open != signalDetected())
  {
    std::string info;
    info.reserve(127);
    SquelchCombine::Node::SquelchStates states;
    for (const auto& state : m_comb->squelchStates(states))
    {
      if (!info.empty())
      {
        info += " ";
      }
      info += state;
    }
    setSignalDetected(is_open, info);
    //setSignalDetected(is_open, m_comb->activityInfo());
  }
} /* SquelchCombine::onSquelchOpen */


bool SquelchCombine::tokenize(const std::string& expr)
{
  string token;
  for (std::string::const_iterator it=expr.begin(); it!=expr.end(); ++it)
  {
    if ((*it == '|') || (*it == '&') || (*it == '(') || (*it == ')') ||
        (*it == '!'))
    {
      if (!token.empty())
      {
        m_tokens.push_back(token);
      }
      token = *it;
      m_tokens.push_back(token);
      token.clear();
    }
    else if (isspace(*it))
    {
      if (!token.empty())
      {
        m_tokens.push_back(token);
      }
      token.clear();
    }
    else if (isprint(*it))
    {
      token += *it;
    }
    else
    {
      std::cout << "*** ERROR: Unusable token with ASCII code "
                << static_cast<unsigned>(static_cast<unsigned char>(*it))
                << " in SQL_COMBINE" << std::endl;
      return false;
    }
  }
  if (!token.empty())
  {
    m_tokens.push_back(token);
  }

  return true;
} /*SquelchCombine::tokenize */


SquelchCombine::Node* SquelchCombine::parseInstExpression(void)
{
  if (m_tokens.empty())
  {
    std::cout << "*** ERROR: Empty squelch combiner expression" << std::endl;
    return nullptr;
  }

  if (m_tokens.front() == "(")
  {
    m_tokens.pop_front();
    Node* expr = parseExpression();
    if ((expr == nullptr) || (m_tokens.front() != ")"))
    {
      delete expr;
      return nullptr;
    }
    m_tokens.pop_front();
    return expr;
  }

  std::string inst(m_tokens.front());
  if ((inst == "|") || (inst == "&") || (inst == ")"))
  {
    std::cout << "*** ERROR: Cannot use operator '" << inst
              << "' as instance name in squelch combiner" << std::endl;
    return nullptr;
  }
  m_tokens.pop_front();

  return new LeafNode(inst);
} /* SquelchCombine::parseInstExpression */


SquelchCombine::Node* SquelchCombine::parseUnaryOpExpression(void)
{
  bool is_negation_op = false;
  if (m_tokens.front() == "!")
  {
    is_negation_op = true;
    m_tokens.pop_front();
  }

  Node* node = parseInstExpression();
  if ((node == nullptr) || !is_negation_op)
  {
    return node;
  }

  return new NegationOpNode(node);
} /* SquelchCombine::parseUnaryOpExpression */


SquelchCombine::Node* SquelchCombine::parseAndExpression(void)
{
  Node* left = parseUnaryOpExpression();
  if ((left == nullptr) || m_tokens.empty() || (m_tokens.front() != "&"))
  {
    return left;
  }

  if (m_tokens.size() < 2)
  {
    std::cout << "*** ERROR: Right hand expression missing in squelch "
                 "combiner AND-expression"
              << std::endl;
    delete left;
    return nullptr;
  }
  m_tokens.pop_front();

  Node* right = parseAndExpression();
  if (right == nullptr)
  {
    delete left;
    return nullptr;
  }

  return new AndOpNode(left, right);
} /* SquelchCombine::parseAndExpression */


SquelchCombine::Node* SquelchCombine::parseOrExpression(void)
{
  Node* left = parseAndExpression();
  if ((left == nullptr) || m_tokens.empty() || (m_tokens.front() != "|"))
  {
    return left;
  }

  if (m_tokens.size() < 2)
  {
    std::cout << "*** ERROR: Right hand expression missing in squelch "
                 "combiner OR-expression"
              << std::endl;
    delete left;
    return nullptr;
  }
  m_tokens.pop_front();

  Node* right = parseOrExpression();
  if (right == nullptr)
  {
    delete left;
    return nullptr;
  }

  return new OrOpNode(left, right);
} /* SquelchCombine::parseOrExpression */


SquelchCombine::Node* SquelchCombine::parseExpression(void)
{
  return parseOrExpression();
} /* SquelchCombine::parseExpresseion */


/*
 * This file has not been truncated
 */

