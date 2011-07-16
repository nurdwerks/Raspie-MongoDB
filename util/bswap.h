
#pragma once

#ifndef MONGO_BSWAP_H
#define MONGO_BSWAP_H

#include "boost/detail/endian.hpp"
#include "boost/static_assert.hpp"
#include <string.h>

namespace mongo {

   // Generic (portable) byte swap function
   template<class T> T byteSwap( T j ) {
       
#ifdef HAVE_BSWAP32
       if ( sizeof( T ) == 4 ) {
           return __builtin_bswap32( j );
       }
#endif
#ifdef HAVE_BSWAP64
       if ( sizeof( T ) == 8 ) {
           return __builtin_bswap64( j );
       }
#endif

      T retVal = 0;
      for ( unsigned i = 0; i < sizeof( T ); ++i ) {
         
         // 7 5 3 1 -1 -3 -5 -7
         int shiftamount = sizeof(T) - 2 * i - 1;
         // 56 40 24 8 -8 -24 -40 -56
         shiftamount *= 8;

         // See to it that the masks can be re-used
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

   template<> inline double byteSwap( double j ) {
      union {
         double d;
         unsigned long long l;
      } u;
      u.d = j;
      u.l = byteSwap<unsigned long long>( u.l );
      return u.d;
   }


   // Here we assume that double is big endian if ints are big endian
   // and also that the format is the same as for x86 when swapped.
   // This is not true for some ARM implementations
   template<class T> T littleEndian( T j ) {
#ifdef BOOST_LITTLE_ENDIAN
      return j;
#else
      return byteSwap<T>(j);
#endif
   }

   template<class T> T bigEndian( T j ) {
#ifdef BOOST_BIG_ENDIAN
      return j;
#else
      return byteSwap<T>(j);
#endif
   }

   BOOST_STATIC_ASSERT( sizeof( double ) == sizeof( unsigned long long ) );

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

#define MONGO_ENDIAN_BODY( MYTYPE, BASE_TYPE, T )                       \
      MYTYPE& operator=( const T& val ) {                               \
          BASE_TYPE::_val = val;                                        \
          return *this;                                                 \
      }                                                                 \
                                                                        \
      operator const T() const {                                        \
          return BASE_TYPE::_val;                                       \
      }                                                                 \
                                                                        \
      MYTYPE& operator+=( T other ) {                                   \
          (*this) = T(*this) + other;                                   \
          return *this;                                                 \
      }                                                                 \
                                                                        \
      MYTYPE& operator-=( T other ) {                                   \
          (*this) = T(*this) - other;                                   \
          return *this;                                                 \
      }                                                                 \
                                                                        \
      MYTYPE& operator&=( T other ) {                                   \
          (*this) = T(*this) & other;                                   \
          return *this;                                                 \
      }                                                                 \
                                                                        \
      MYTYPE& operator|=( T other ) {                                   \
          (*this) = T(*this) | other;                                   \
          return *this;                                                 \
      }                                                                 \
                                                                        \
      MYTYPE& operator^=( T other ) {                                   \
          (*this) = T(*this) ^ other;                                   \
          return *this;                                                 \
      }                                                                 \
                                                                        \
      MYTYPE& operator++() {                                            \
          return (*this) += 1;                                          \
      }                                                                 \
                                                                        \
      MYTYPE operator++(int) {                                          \
          MYTYPE old = *this;                                           \
          ++(*this);                                                    \
          return old;                                                   \
      }                                                                 \
                                                                        \
      MYTYPE& operator--() {                                            \
          return (*this) -= 1;                                          \
      }                                                                 \
                                                                        \
      MYTYPE operator--(int) {                                          \
          MYTYPE old = *this;                                           \
          --(*this);                                                    \
          return old;                                                   \
      }                                                                 \
                                                                        \
      friend std::ostream& operator<<( std::ostream& ost, MYTYPE val ) { \
          return ost << T(val);                                         \
      }                                                                 \
                                                                        \
      friend Nullstream& operator<<( Nullstream& ost, MYTYPE val ) {    \
          return ost << T(val);                                         \
      }
  

  template<class T, class S = aligned_storage<T> > class storageLE {
  private:
     S _val;
     typedef storageLE<T,S> this_type;
  public:
      storageLE() {}

      storageLE( T val ) {
         (*this) = val;
      }
  
      MONGO_ENDIAN_BODY( storageLE, this_type, T );
  };

  template<class T, class D> void storeLE( D* dest, T src ) {
#if defined(BOOST_LITTLE_ENDIAN) || !defined( ALIGNMENT_IMPORTANT )
      // This also assumes no alignment issues
      *dest = littleEndian<T>( src );
#else
      unsigned char* u_dest = reinterpret_cast<unsigned char*>( dest );
      for ( unsigned i = 0; i < sizeof( T ); ++i ) {
         u_dest[i] = src >> ( 8 * i );
      }
#endif
   }
   
  template<class T, class S> T loadLE( const S* data ) {
#if defined(BOOST_LITTLE_ENDIAN) || !defined( ALIGNMENT_IMPORTANT )
      return littleEndian<T>( *data );
#else
      T retval = 0;
      const unsigned char* u_data = reinterpret_cast<const unsigned char*>( data );
      for( unsigned i = 0; i < sizeof( T ); ++i ) {
          retval |= T( u_data[i] ) << ( 8 * i );
      }
      return retval;
#endif
  }

  template<class T, class D> void store_big( D* dest, T src ) {
#if defined(BOOST_BIG_ENDIAN) || !defined( ALIGNMENT_IMPORTANT )
      // This also assumes no alignment issues
      *dest = bigEndian<T>( src );
#else
      unsigned char* u_dest = reinterpret_cast<unsigned char*>( dest );
      for ( unsigned i = 0; i < sizeof( T ); ++i ) {
          u_dest[ sizeof(T) - 1 - i ] = src >> ( 8 * i );
      }
#endif
   }
   
  template<class T, class S> T load_big( const S* data ) {
#if defined(BOOST_BIG_ENDIAN) || !defined( ALIGNMENT_IMPORTANT )
      return bigEndian<T>( *data );
#else
      T retval = 0;
      const unsigned char* u_data = reinterpret_cast<const unsigned char*>( data );
      for( unsigned i = 0; i < sizeof( T ); ++i ) {
          retval |= T( u_data[ sizeof(T) - 1 - i ] ) << ( 8 * i );
      }
      return retval;
#endif
  }


  /** Holds the actual type of storage */
  template<typename T> class storage_type {
  public:
      typedef T t;
  };

  template<> class storage_type<bool> {
  public:
      typedef unsigned char t;
  };

  template<> class storage_type<double> {
  public:
      typedef unsigned long long t;
  };


#pragma pack(1)
  template<class T> struct packed_little_storage {
  private:
      typedef typename storage_type<T>::t S;
      S _val;
  public:
      
     packed_little_storage& operator=( T val ) {
        storeLE<S>( &_val, convert<T,S>( val ) );
        return *this;
     }
     
     operator T() const {
        return convert<S,T>( loadLE<S>( &_val ) );
     }
  } __attribute__((packed)) ;

  template<class T> struct packed_big_storage {
  private:
      typedef typename storage_type<T>::t S;
      S _val;
  public:
      
     packed_big_storage& operator=( T val ) {
        store_big<S>( &_val, convert<T,S>( val ) );
        return *this;
     }
     
     operator T() const {
        return convert<S,T>( load_big<S>( &_val ) );
     }
  } __attribute__((packed)) ;


  template<class T> struct packedLE {
     typedef storageLE<T, packed_little_storage<T> > t;
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

#define MONGO_ENDIAN_REF_FUNCS( TYPE )                          \
      static TYPE& ref( void* src ) {                           \
          return *reinterpret_cast<TYPE*>( src );               \
      }                                                         \
                                                                \
      static const TYPE& ref( const void* src ) {               \
          return *reinterpret_cast<const TYPE*>( src );         \
      }

  template<class T> class little_pod {
  protected:
      packed_little_storage<T> _val;
  public:
      MONGO_ENDIAN_REF_FUNCS( little_pod );
      MONGO_ENDIAN_BODY( little_pod, little_pod<T>, T );
  };

  template<class T> class little : public little_pod<T> {
  public:
      inline little( T x ) {
          *this = x;
      }

      inline little() {}
      MONGO_ENDIAN_REF_FUNCS( little );
      MONGO_ENDIAN_BODY( little, little_pod<T>, T );
  };

  template<class T> class big_pod {
  protected:
      packed_big_storage<T> _val;
  public:
      MONGO_ENDIAN_REF_FUNCS( big_pod );
      MONGO_ENDIAN_BODY( big_pod, big_pod<T>, T );
  };

  template<class T> class big : public big_pod<T> {
  public:
      inline big( T x ) {
          *this = x;
      }

      inline big() {}
      MONGO_ENDIAN_REF_FUNCS( big );
      MONGO_ENDIAN_BODY( big, big_pod<T>, T );
  };

  template<class T> T readBE( const void* data ) {
      return big<T>::ref( data );
  }

  template<class T> void copyBE( void* dest, T src ) {
      big<T>::ref( dest ) = src;
  }


  BOOST_STATIC_ASSERT( sizeof( little_pod<double> ) == 8 );
  BOOST_STATIC_ASSERT( sizeof( little<double> ) == 8 );
  BOOST_STATIC_ASSERT( sizeof( big<bool> ) == 1 );

}

#endif
