#include "phonenumber.h"

using namespace KABC;

PhoneNumber::PhoneNumber() :
  mType( Home )
{
}

PhoneNumber::PhoneNumber( Type type, const QString &number ) :
  mType( type ), mNumber( number )
{
}

PhoneNumber::~PhoneNumber()
{
}

void PhoneNumber::setNumber( const QString &number )
{
  mNumber = number;
}

QString PhoneNumber::number() const
{
  return mNumber;
}

void PhoneNumber::setType( PhoneNumber::Type type )
{
  mType = type;
}

PhoneNumber::Type PhoneNumber::type() const
{
  return mType;
}
