

#ifndef MONGO_BSWAP_H
#define MONGO_BSWAP_H

#include "boost/detail/endian.hpp"
#include <string.h>

namespace mongo {

   template<class T> T byteSwap( T j ) {
      T retVal = 0;
      for( unsigned i = 0; i < sizeof( T ); ++i ) {
         retVal <<= 8;
         retVal |= ( j & 0xff );
         j >>= 8;
      }
      return retVal;
   }

   template<class T> T littleEndian( T j ) {
#ifdef BOOST_LITTLE_ENDIAN
      return j;
#else
      return byteSwap<T>(j);
#endif
   }

   template<> inline double littleEndian( double j ) {
      unsigned long long* toSwap = reinterpret_cast<unsigned long long*>( &j );
      *toSwap = littleEndian<unsigned long long>( *toSwap );
      return *reinterpret_cast<double*>( toSwap );
   }
  
   template<class T> void copyLE( char* dest, T src ) {
#if defined(BOOST_LITTLE_ENDIAN) && !defined( ALIGNMENT_IMPORTANT )
      // This also assumes no alignment issues
      *reinterpret_cast<T*>(dest) = src;
#else
      for ( unsigned i = 0; i < sizeof( T ); ++i ) {
         dest[i] = src >> ( 8 * i );
      }
#endif
   }
 
   template<> inline void copyLE<double>( char* dest, double src ) {
      copyLE<unsigned long long>( dest, *reinterpret_cast<unsigned long long*>( &src ) );
   }

   template<> inline void copyLE<bool>( char* dest, bool src ) {
      *dest = src;
   }

   template<class T> void copyLE( void* dest, T src ) {
      copyLE<T>( reinterpret_cast<char*>( dest ), src );
   }
   
   template<class T> T readLE( const char* data ) {
#if defined(BOOST_LITTLE_ENDIAN) && !defined( ALIGNMENT_IMPORTANT )
      return *reinterpret_cast<const T*>( data );
#else
      T retval = 0;
      const unsigned char* u_data = reinterpret_cast<const unsigned char*>( data );
      for( unsigned i = 0; i < sizeof( T ); ++i ) {
         retval |= T( u_data[i] ) << ( 8 * i );
      }
      return retval;
#endif
  }

  template<> inline double readLE<double>( const char* data ) {
     unsigned long long tmp = readLE<unsigned long long>( data );
     return *reinterpret_cast<double*>( &tmp );
  }
  
  class Nullstream;

  template<class T> class storageLE {
  private:
      T _val;

  public:
      storageLE() {}
 
      storageLE( T val ) {
         _val = littleEndian<T>( val );
      }

      storageLE& operator=( int val ) {
         _val = littleEndian<T>( val );
         return *this;
      }

      operator const T() const {
         return littleEndian<T>( _val );
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

     friend ostream& operator<<( ostream& ost, storageLE val ) {
        return ost << T(val);
     }

     friend Nullstream& operator<<( Nullstream& ost, storageLE val ) {
        return ost << T(val);
     }

  };
}

#endif
