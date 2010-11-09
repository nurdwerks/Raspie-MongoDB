
#pragma once

#ifndef MONGO_BSWAP_H
#define MONGO_BSWAP_H

#include "boost/detail/endian.hpp"
#include "boost/static_assert.hpp"
#include <string.h>

namespace mongo {

   // Generic (portable) byte swap function
   template<class T> T byteSwap( T j ) {      
      T retVal = 0;
      for ( unsigned i = 0; i < sizeof( T ); ++i ) {
         
         // 7 5 3 1 -1 -3 -5 -7
         int shiftamount = sizeof(T) - 2 * i - 1;
         // 56 40 24 8 -8 -24 -40 -56
         shiftamount *= 8;
         if ( shiftamount > 0 ) {
            T mask = T( 0xff ) << ( 8 * i );
            retVal |= ( (j & mask ) << shiftamount );
         } else {
            T mask = T( 0xff ) << ( 8 * (sizeof(T) - i - 1) );
            retVal |= ( j >> -shiftamount ) & mask;
         }
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

   template<class S, class D> D convert( S src )
   {
      union { S s; D d; } u;
      u.s = src;
      return u.d;
   }

   template<> inline char convert<bool,char>( bool src ) {
      return src;
   }

   template<> inline bool convert<char, bool>( char src ) {
      return src;
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

   template<class T> void storeLE( void* dest, T src ) {
#if defined(BOOST_LITTLE_ENDIAN) || !defined( ALIGNMENT_IMPORTANT )
      // This also assumes no alignment issues
      *reinterpret_cast<T*>(dest) = littleEndian<T>( src );
#else
      unsigned char* u_dest = reinterpret_cast<unsigned char*>( dest );
      for ( unsigned i = 0; i < sizeof( T ); ++i ) {
         u_dest[i] = src >> ( 8 * i );
      }
#endif
   }
   
  template<class T> T loadLE( const void* data ) {
#if defined(BOOST_LITTLE_ENDIAN) || !defined( ALIGNMENT_IMPORTANT )
     return littleEndian<T>( *reinterpret_cast<const T*>( data ) );
#else
      T retval = 0;
      const unsigned char* u_data = reinterpret_cast<const unsigned char*>( data );
      for( unsigned i = 0; i < sizeof( T ); ++i ) {
         retval |= T( u_data[i] ) << ( 8 * i );
      }
      return retval;
#endif
  }

#pragma pack(1)
  template<class T, class S> struct packed_storage {
  private:
     S _val;
  public:
     packed_storage() {
     }

     packed_storage& operator=( T val ) {
        storeLE<S>( &_val, convert<T,S>( val ) );
        return *this;
     }

     packed_storage( T val ) {
        *this = val;
     }

     operator T() const {
        return convert<S,T>( loadLE<S>( &_val ) );
     }
  } __attribute__((packed)) ;

  template<class T> struct packedLE {
     typedef storageLE<T, packed_storage<T,T> > t;
  };

  template<> struct packedLE<double> {
     typedef storageLE<double, packed_storage<double, unsigned long long> > t;
  };

  template<> struct packedLE<bool> {
     typedef storageLE<bool, packed_storage<bool, char> > t;
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

  template<class T> T readBE( const void* data ) {
      T retval = 0;
      const unsigned char* u_data = reinterpret_cast<const unsigned char*>( data );
      for( unsigned i = 0; i < sizeof( T ); ++i ) {
         retval |= T( u_data[i] ) << ( 8 * ( sizeof( T ) - 1 - i ) );
      }
      return retval;
  }

  template<class T> void copyBE( void* dest, T src ) {
     unsigned char* u_dest = reinterpret_cast<unsigned char*>( dest );
     for ( unsigned i = 0; i < sizeof( T ); ++i ) {
        u_dest[ sizeof(T) - 1 - i ] = src >> ( 8 * i );
     }
  }

}

#endif
