/**
*    Copyright (C) 2008 10gen Inc.
*
*    This program is free software: you can redistribute it and/or  modify
*    it under the terms of the GNU Affero General Public License, version 3,
*    as published by the Free Software Foundation.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "diskloc.h"
#include "jsobj.h"
#include "namespace-inl.h"
#include "../util/message.h"
#include "../client/constants.h"

namespace mongo {

    /* db response format

       Query or GetMore: // see struct QueryResult
          int resultFlags;
          int64 cursorID;
          int startingFrom;
          int nReturned;
          list of marshalled JSObjects;
    */

    extern bool objcheck;

#pragma pack(1)
    struct QueryResult : public MsgData {
        packedLE<long long>::t cursorId;
        packedLE<int>::t       startingFrom;
        packedLE<int>::t       nReturned;

        const char *data() {
            return reinterpret_cast<char*>( &nReturned + 1 );
        }

        packedLE<int>::t& _resultFlags() {
           return dataAsInt();
        }

        int resultFlags() const {
            return getDataAsInt();
        }
        
        void setResultFlagsToOk() {
            _resultFlags() = ResultFlag_AwaitCapable;
        }

        void setResultFlags( int resultFlags ) {
           setDataAsInt( resultFlags );
        }
    };
#pragma pack()

    /* For the database/server protocol, these objects and functions encapsulate
       the various messages transmitted over the connection.

       See http://www.mongodb.org/display/DOCS/Mongo+Wire+Protocol
    */
    class DbMessage {
    public:
        DbMessage(const Message& _m) : m(_m) , mark(0) {
            // for received messages, Message has only one buffer
            theEnd = _m.singleData()->_data + _m.header()->dataLen();
            char *r = _m.singleData()->_data;
            reserved = &refLE<int>( r );
            data = r + 4;
            nextjsobj = data;
        }

        /** the 32 bit field before the ns */
        packedLE<int>::t& reservedField() { return *reserved; }

        const char * getns() const {
            return data;
        }
        void getns(Namespace& ns) const {
            ns = data;
        }

        const char * afterNS() const {
            return data + strlen( data ) + 1;
        }

        int getInt( int num ) const {
            return readLE<int>( afterNS() + 4 * num );
        }

        int getQueryNToReturn() const {
            return getInt( 1 );
        }

        void resetPull(){ nextjsobj = data; }
        int pullInt() const { return pullInt(); }

        packedLE<int>::t& pullInt() {
            if ( nextjsobj == data )
                nextjsobj += strlen(data) + 1; // skip namespace
            packedLE<int>::t& i = refLE<int>(nextjsobj);
            nextjsobj += 4;
            return i;
        }

        /**
         * get an int64 at specified offsetBytes after ns
         */
        long long getInt64() {
            if ( nextjsobj == data )
                nextjsobj += strlen(data) + 1; // skip namespace
            return readLE<long long>( nextjsobj );
        }

        long long pullInt64() {
           long long ret = getInt64();
           nextjsobj += 8;
           return ret;
        }

        void pushInt64( long long toPush ) {
           if ( nextjsobj == data )
              nextjsobj += strlen(data) + 1; // skip namespace
           copyLE<long long>( const_cast<char*>(nextjsobj), toPush );
           nextjsobj += 8;
        }

        OID* getOID() const {
            return (OID *) (data + strlen(data) + 1); // skip namespace
        }

        void getQueryStuff(const char *&query, int& ntoreturn) {
            query = data + strlen(data) + 1;
            ntoreturn = readLE<int>( query );
            query += 4;
        }

        /* for insert and update msgs */
        bool moreJSObjs() const {
            return nextjsobj != 0;
        }
        BSONObj nextJsObj() {
            if ( nextjsobj == data ) {
                nextjsobj += strlen(data) + 1; // skip namespace
                massert( 13066 ,  "Message contains no documents", theEnd > nextjsobj );
            }
            massert( 10304 ,  "Client Error: Remaining data too small for BSON object", theEnd - nextjsobj > 3 );
            BSONObj js(nextjsobj);
            massert( 10305 ,  "Client Error: Invalid object size", js.objsize() > 3 );
            massert( 10306 ,  "Client Error: Next object larger than space left in message",
                     js.objsize() < ( theEnd - data ) );
            if ( objcheck && !js.valid() ) {
                massert( 10307 , "Client Error: bad object in message", false);
            }
            nextjsobj += js.objsize();
            if ( nextjsobj >= theEnd )
                nextjsobj = 0;
            return js;
        }

        const Message& msg() const { return m; }

        void markSet() {
            mark = nextjsobj;
        }

        void markReset() {
            assert( mark );
            nextjsobj = mark;
        }

    private:
        const Message& m;
        packedLE<int>::t* reserved;
        const char *data;
        const char *nextjsobj;
        const char *theEnd;

        const char * mark;
    };


    /* a request to run a query, received from the database */
    class QueryMessage {
    public:
        const char *ns;
        int ntoskip;
        int ntoreturn;
        int queryOptions;
        BSONObj query;
        BSONObj fields;

        /* parses the message into the above fields */
        QueryMessage(DbMessage& d) {
            ns = d.getns();
            ntoskip = d.pullInt();
            ntoreturn = d.pullInt();
            query = d.nextJsObj();
            if ( d.moreJSObjs() ) {
                fields = d.nextJsObj();
            }
            queryOptions = d.msg().header()->getDataAsInt();
        }
    };

} // namespace mongo

#include "../client/dbclient.h"

namespace mongo {

    inline void replyToQuery(int queryResultFlags,
                             AbstractMessagingPort* p, Message& requestMsg,
                             void *data, int size,
                             int nReturned, int startingFrom = 0,
                             long long cursorId = 0
                            ) {
        BufBuilder b(32768);
        b.skip(sizeof(QueryResult));
        b.appendBuf(data, size);
        QueryResult *qr = (QueryResult *) b.buf();
        qr->setResultFlags( queryResultFlags );
        qr->len = b.len();
        qr->setOperation( opReply );
        qr->cursorId = cursorId;
        qr->startingFrom = startingFrom;
        qr->nReturned = nReturned;
        b.decouple();
        Message resp(qr, true);
        p->reply(requestMsg, resp, requestMsg.header()->id);
    }

} // namespace mongo

//#include "bsonobj.h"

#include "instance.h"

namespace mongo {

    /* object reply helper. */
    inline void replyToQuery(int queryResultFlags,
                             AbstractMessagingPort* p, Message& requestMsg,
                             BSONObj& responseObj) {
        replyToQuery(queryResultFlags,
                     p, requestMsg,
                     (void *) responseObj.objdata(), responseObj.objsize(), 1);
    }

    /* helper to do a reply using a DbResponse object */
    inline void replyToQuery(int queryResultFlags, Message &m, DbResponse &dbresponse, BSONObj obj) {
        BufBuilder b;
        b.skip(sizeof(QueryResult));
        b.appendBuf((void*) obj.objdata(), obj.objsize());
        QueryResult* msgdata = (QueryResult *) b.buf();
        b.decouple();
        QueryResult *qr = msgdata;
        qr->setResultFlags( queryResultFlags );
        qr->len = b.len();
        qr->setOperation( opReply );
        qr->cursorId = 0;
        qr->startingFrom = 0;
        qr->nReturned = 1;
        Message *resp = new Message();
        resp->setData(msgdata, true); // transport will free
        dbresponse.response = resp;
        dbresponse.responseTo = m.header()->id;
    }

    string debugString( Message& m );

} // namespace mongo
