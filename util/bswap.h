
#pragma once

#ifndef MONGO_BSWAP_H
#define MONGO_BSWAP_H

#include "boost/detail/endian.hpp"
#include "boost/static_assert.hpp"
#include <string.h>

namespace mongo {

   // Generic (portable) byte swap function
   template<class T> T byteSwap( T j ) {      
      T retVal = j;
      for( int i = 0; i < int(sizeof( T )) - 1; ++i ) {
         retVal <<= 8;
         j >>= 8;
         retVal |= j & 0xff;
      }
      return retVal;
   }

   template<class T> T littleEndian( T j ) {
#ifdef BOOST_LITTLE_ENDIAN
      return j;
#else
      return byteSwap<T>(j);
   }

   BOOST_STATIC_ASSERT( sizeof( double ) == sizeof( unsigned long long ) );

   // Here we assume that double is big endian if ints are big endian
   // and also that the format is the same as for x86 when swapped.
   template<> inline double littleEndian( double j ) {
      union {
         double d;
         unsigned long long l;
      } u;
      u.d = j;
      u.l = littleEndian<unsigned long long>( u.l );
      return u.d;
#endif
   }

  class Nullstream;

  template<class T> struct aligned_storage {
  private:
     T _val;
  public:
     aligned_storage() {
     }

     aligned_storage( T val ) {
        _val = littleEndian<T>(val);
     }

     operator T() const {
        return littleEndian<T>(_val);
     }
  };

  template<> struct aligned_storage<bool> {
  private:
     char _val;
  };


  template<class T, class S = aligned_storage<T> > class storageLE {
  private:
      S _val;

  public:
      storageLE() {}

      storageLE& operator=( T val ) {
         _val = val;
         return *this;
      }

      storageLE( T val ) {
         (*this) = val;
      }

      operator const T() const {
         return _val;
      }

      storageLE& operator+=( T other ) {
         (*this) = T(*this) + other;
         return *this;
      }

      storageLE& operator-=( T other ) {
         (*this) = T(*this) - other;
         return *this;
      }

      storageLE& operator&=( T other ) {
         (*this) = T(*this) & other;
         return *this;
      }

      storageLE& operator|=( T other ) {
         (*this) = T(*this) | other;
         return *this;
      }

     storageLE& operator++() {
         return (*this) += 1;
     }

     storageLE operator++(int) {
         storageLE old = *this;
         ++(*this);
         return old;
     }

     storageLE& operator--() {
         return (*this) -= 1;
     }

     storageLE operator--(int) {
         storageLE old = *this;
         --(*this);
         return old;
     }

     friend std::ostream& operator<<( std::ostream& ost, storageLE val ) {
        return ost << T(val);
     }

     friend Nullstream& operator<<( Nullstream& ost, storageLE val ) {
        return ost << T(val);
     }

  };

#pragma pack(1)
  template<class T> struct packed_storage {
  private:
     T _val;
  public:
     packed_storage() {
     }

     packed_storage( T val ) {        
        _val = littleEndian<T>( val );
     }

     operator T() const {
        return littleEndian<T>( _val );
     }
  } __attribute__((packed)) ;

  template<> struct packed_storage<bool> {
  private:
     char _val;
  };

  template<class T> struct packedLE {
     typedef storageLE<T, packed_storage<T> > t;
  };

  BOOST_STATIC_ASSERT( sizeof( packedLE<bool>::t ) == 1 );
#pragma pack()
  
  // Helper functions which will replace readLE. Can be used as pointers as well
  template<class T> const typename packedLE<T>::t& refLE( const void* data ) {
     return *reinterpret_cast<const typename packedLE<T>::t*>( data );
  }
  
  template<class T> typename packedLE<T>::t& refLE( void* data ) {
     return *reinterpret_cast<typename packedLE<T>::t*>( data );
  }

  template<class T> void copyLE( char* dest, T src ) {
      refLE<T>( dest ) = src;
  }

  template<class T> void copyLE( void* dest, T src ) {
     copyLE<T>( reinterpret_cast<char*>( dest ), src );
  }

  template<class T> T readLE( const char* data ) {
     return refLE<T>( data );
  }
}

#endif
