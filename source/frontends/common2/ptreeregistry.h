#pragma once

#include "linux/registry.h"
#include <boost/property_tree/ptree.hpp>
#include <string>

namespace common2
{

  class PTreeRegistry : public Registry
  {
  public:
    struct KeyQtEncodedLess
    {
      // this function is used to make the Qt registry compatible with sa2 and napple
      // it is here, in the base class PTreeRegistry simply because it makes things easier
      // KeyQtEncodedLess goes in the typedef init_t below
      bool operator()(const std::string & lhs, const std::string & rhs) const;
    };

    typedef boost::property_tree::basic_ptree<std::string, std::string, KeyQtEncodedLess> ini_t;

    std::string getString(const std::string & section, const std::string & key) const override;
    DWORD getDWord(const std::string & section, const std::string & key) const override;
    bool getBool(const std::string & section, const std::string & key) const override;

    void putString(const std::string & section, const std::string & key, const std::string & value) override;
    void putDWord(const std::string & section, const std::string & key, const DWORD value) override;

    template<typename T>
    T getValue(const std::string & section, const std::string & key) const;

    template<typename T>
    void putValue(const std::string & section, const std::string & key, const T & value);

  protected:
    ini_t myINI;
  };

}
