/*
 Rocrail - Model Railroad Software

 Copyright (C) 2002-2014 Rob Versluis, Rocrail.net

 


 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 3
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/


#include "rocrail/impl/operator_impl.h"

#include "rocrail/public/app.h"
#include "rocrail/public/car.h"
#include "rocrail/public/model.h"
#include "rocrail/public/loc.h"

#include "rocs/public/mem.h"
#include "rocs/public/strtok.h"

#include "rocrail/wrapper/public/Operator.h"
#include "rocrail/wrapper/public/Loc.h"
#include "rocrail/wrapper/public/Car.h"

static int instCnt = 0;

static void __informLocos(iOOperator inst);


/** ----- OBase ----- */
static void __del( void* inst ) {
  if( inst != NULL ) {
    iOOperatorData data = Data(inst);
    /* Cleanup data->xxx members...*/

    freeMem( data );
    freeMem( inst );
    instCnt--;
  }
  return;
}

static const char* __name( void ) {
  return name;
}

static unsigned char* __serialize( void* inst, long* size ) {
  return NULL;
}

static void __deserialize( void* inst,unsigned char* bytestream ) {
  return;
}

static char* __toString( void* inst ) {
  return NULL;
}

static int __count( void ) {
  return instCnt;
}

static struct OBase* __clone( void* inst ) {
  return NULL;
}

static Boolean __equals( void* inst1, void* inst2 ) {
  return False;
}

static void* __properties( void* inst ) {
  iOOperatorData data = Data((iOOperator)inst);
  return data->props;
}

static const char* __id( void* inst ) {
  iOOperatorData data = Data((iOOperator)inst);
  return wOperator.getid(data->props);
}

static void* __event( void* inst, const void* evt ) {
  return NULL;
}

/** ----- OOperator ----- */


/**  */
static char* _getForm( void* object ) {
  return 0;
}


/**  */
static char* _postForm( void* object ,const char* data ) {
  return 0;
}


/**  */
static const char* _tableHdr( void ) {
  return 0;
}


/**  */
static char* _toHtml( void* object ) {
  return 0;
}


/**  */
static struct OOperator* _inst( iONode ini ) {
  iOOperator __Operator = allocMem( sizeof( struct OOperator ) );
  iOOperatorData data = allocMem( sizeof( struct OOperatorData ) );
  MemOp.basecpy( __Operator, &OperatorOp, 0, sizeof( struct OOperator ), data );

  /* Initialize data->xxx members... */
  data->props = ini;

  instCnt++;
  return __Operator;
}


static Boolean _cmd( iOOperator inst, iONode nodeA ) {
  iOOperatorData data = Data(inst);

  const char* nodename = NodeOp.getName( nodeA );
  const char* cmd      = wOperator.getcmd( nodeA );

  TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "command %s:%s for operator %s",
      nodename, (cmd==NULL?"-":cmd), wOperator.getid(data->props) );

  if( StrOp.equals( wLoc.block, cmd ) ) {
    OperatorOp.setLocality(inst, wOperator.getlocation(nodeA));
  }

  else if( StrOp.equals( wOperator.emptycar, cmd ) ) {
    /* empty the carids */
    iOStrTok tok = StrTokOp.inst(wOperator.getcarids(nodeA), ',');
    while( StrTokOp.hasMoreTokens(tok) ) {
      const char* carid = StrTokOp.nextToken(tok);
      if( OperatorOp.hasCar(inst, carid) ) {
        iOCar car = ModelOp.getCar(AppOp.getModel(), carid);
        if( car != NULL ) {
          iONode cmd = NodeOp.inst( wCar.name(), NULL, ELEMENT_NODE );
          wCar.setcmd( cmd, wCar.status_empty );
          CarOp.cmd( car, cmd );
        }
      }
    }
    StrTokOp.base.del(tok);
    __informLocos(inst);
  }

  else if( StrOp.equals( wOperator.emptycar, cmd ) ) {
    /* load the carids */
    iOStrTok tok = StrTokOp.inst(wOperator.getcarids(nodeA), ',');
    while( StrTokOp.hasMoreTokens(tok) ) {
      const char* carid = StrTokOp.nextToken(tok);
      if( OperatorOp.hasCar(inst, carid) ) {
        iOCar car = ModelOp.getCar(AppOp.getModel(), carid);
        if( car != NULL ) {
          iONode cmd = NodeOp.inst( wCar.name(), NULL, ELEMENT_NODE );
          wCar.setcmd( cmd, wCar.status_loaded );
          CarOp.cmd( car, cmd );
        }
      }
    }
    StrTokOp.base.del(tok);
    __informLocos(inst);
  }

  else if( StrOp.equals( wOperator.addcar, cmd ) ) {
    /* add the carids */
    iOStrTok tok = StrTokOp.inst(wOperator.getcarids(nodeA), ',');
    while( StrTokOp.hasMoreTokens(tok) ) {
      const char* carid = StrTokOp.nextToken(tok);
      if( !OperatorOp.hasCar(inst, carid) ) {
        /* add the car */
        char* newCarIds = NULL;
        if( StrOp.len(wOperator.getcarids(data->props)) > 0 ) {
          newCarIds = StrOp.fmt("%s,%s", wOperator.getcarids(data->props), carid);
        }
        else {
          newCarIds = StrOp.fmt("%s", carid);
        }
        TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "add car [%s] to operator [%s]: [%s]", carid, wOperator.getid(data->props), newCarIds );
        wOperator.setcarids(data->props, newCarIds);
        StrOp.free(newCarIds);
      }
    }
    StrTokOp.base.del(tok);
    /* Broadcast to clients. */
    {
      iONode clone = (iONode)NodeOp.base.clone( data->props );
      AppOp.broadcastEvent( clone );
    }
    __informLocos(inst);
  }

  else if( StrOp.equals( wOperator.removecar, cmd ) ) {
    /* remove the carids */
    iOStrTok tok = StrTokOp.inst(wOperator.getcarids(nodeA), ',');
    while( StrTokOp.hasMoreTokens(tok) ) {
      const char* carid = StrTokOp.nextToken(tok);
      if( OperatorOp.hasCar(inst, carid) ) {
        char* newCarIds = NULL;
        iOStrTok cars = StrTokOp.inst(wOperator.getcarids(data->props), ',');
        TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "remove car [%s] to operator [%s]", carid, wOperator.getid(data->props) );
        while( StrTokOp.hasMoreTokens(cars) ) {
          const char* oldcarid = StrTokOp.nextToken(cars);
          if( !StrOp.equals(oldcarid, carid) ) {
            if( newCarIds != NULL && StrOp.len(newCarIds) > 0 ) {
              newCarIds = StrOp.cat(newCarIds, ",");
            }
            newCarIds = StrOp.cat(newCarIds, oldcarid);
          }
        }
        StrTokOp.base.del(cars);
        TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "operator [%s] cars=[%s]", wOperator.getid(data->props), newCarIds );
        wOperator.setcarids(data->props, newCarIds);
        StrOp.free(newCarIds);
      }
    }
    StrTokOp.base.del(tok);
    /* Broadcast to clients. */
    {
      iONode clone = (iONode)NodeOp.base.clone( data->props );
      AppOp.broadcastEvent( clone );
    }
    __informLocos(inst);
  }

  else {
    iOStrTok tok = StrTokOp.inst(wOperator.getcarids(data->props), ',');
    while( StrTokOp.hasMoreTokens(tok) ) {
      iOCar car = ModelOp.getCar(AppOp.getModel(), StrTokOp.nextToken(tok) );
      if( car != NULL ) {
        CarOp.cmd(car, (iONode)NodeOp.base.clone(nodeA));
      }
    }
    StrTokOp.base.del(tok);
  }

  nodeA->base.del(nodeA);
  return True;
}

static void _swapPlacing( iOOperator inst, iONode cmd ) {
  iOOperatorData data = Data(inst);
  iOStrTok tok = StrTokOp.inst(wOperator.getcarids(data->props), ',');

  TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "swap for operator %s", wOperator.getid(data->props) );

  while( StrTokOp.hasMoreTokens(tok) ) {
    iOCar car = ModelOp.getCar(AppOp.getModel(), StrTokOp.nextToken(tok) );
    if( car != NULL ) {
      CarOp.swapPlacing(car, cmd);
    }
  }
  StrTokOp.base.del(tok);
}


static void __informLocos(iOOperator inst) {
  iOOperatorData data = Data(inst);
  /* Inform locos */
  iOMap lcMap = ModelOp.getLocoMap(AppOp.getModel());
  iOLoc lc = (iOLoc)MapOp.first( lcMap );

  while( lc != NULL ) {
    iONode props = LocOp.base.properties(lc);
    if( StrOp.equals(wOperator.getid(data->props), wLoc.gettrain(props) )) {
      LocOp.getLen(lc);
      AppOp.broadcastEvent( (iONode)props->base.clone( props ) );
    }
    lc = (iOLoc)MapOp.next( lcMap );
  };
}

/**  */
static void _modify( struct OOperator* inst ,iONode props ) {
  iOOperatorData data = Data(inst);

  if( props != NULL ) {
    int cnt = NodeOp.getAttrCnt( props );
    int i = 0;
    for( i = 0; i < cnt; i++ ) {
      iOAttr attr = NodeOp.getAttr( props, i );
      const char* name  = AttrOp.getName( attr );
      const char* value = AttrOp.getVal( attr );
      NodeOp.setStr( data->props, name, value );
    }
    props->base.del(props);
  }

  /* Inform locos */
  __informLocos(inst);

  /* Broadcast to clients. */
  {
    iONode clone = (iONode)NodeOp.base.clone( data->props );
    AppOp.broadcastEvent( clone );
  }
}


static Boolean _hasCar( struct OOperator* inst, const char* id ) {
  iOOperatorData data = Data(inst);
  Boolean hascar = False;
  iOStrTok tok = StrTokOp.inst(wOperator.getcarids(data->props), ',');
  while( StrTokOp.hasMoreTokens(tok) ) {
    if( StrOp.equals(id, StrTokOp.nextToken(tok)) ) {
      hascar = True;
      break;
    }
  }
  StrTokOp.base.del(tok);
  return hascar;
}


static Boolean _isCommuter( struct OOperator* inst ) {
  iOOperatorData data = Data(inst);
  Boolean commuter = False;
  iOStrTok tok = StrTokOp.inst(wOperator.getcarids(data->props), ',');
  while( StrTokOp.hasMoreTokens(tok) ) {
    iOCar car = ModelOp.getCar(AppOp.getModel(), StrTokOp.nextToken(tok) );
    if( car != NULL && CarOp.isCommuter(car) ) {
      commuter = True;
      break;
    }
  }
  StrTokOp.base.del(tok);
  return commuter;
}


static const char* _getCargo( struct OOperator* inst ) {
  iOOperatorData data = Data(inst);
  return wOperator.getcargo(data->props);
}


static const char* _getClass( struct OOperator* inst ) {
  iOOperatorData data = Data(inst);
  return wOperator.getclass(data->props);
}


static int _getVMax( struct OOperator* inst ) {
  iOOperatorData data = Data(inst);
  int vmax = wOperator.getV_max(data->props);
  iOStrTok tok = StrTokOp.inst(wOperator.getcarids(data->props), ',');
  while( StrTokOp.hasMoreTokens(tok) ) {
    iOCar car = ModelOp.getCar(AppOp.getModel(), StrTokOp.nextToken(tok) );
    if( car != NULL ) {
      int vmaxcar = CarOp.getVMax(car);
      if( vmaxcar > 0 ) {
        if( vmax == 0 || vmaxcar < vmax)
          vmax = vmaxcar;
      }
    }
  }
  StrTokOp.base.del(tok);

  return vmax;
}


static void _setLocality( struct OOperator* inst, const char* bkid ) {
  iOOperatorData data = Data(inst);
  iOStrTok tok = StrTokOp.inst(wOperator.getcarids(data->props), ',');
  TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "set locality to %s for operator %s", bkid==NULL?"-":bkid, wOperator.getid(data->props) );
  while( StrTokOp.hasMoreTokens(tok) ) {
    iOCar car = ModelOp.getCar(AppOp.getModel(), StrTokOp.nextToken(tok) );
    if( car != NULL ) {
      CarOp.setLocality(car, bkid);
    }
  }
  StrTokOp.base.del(tok);
  wOperator.setlocation(data->props, bkid);

  {
    iONode clone = (iONode)NodeOp.base.clone( data->props );
    AppOp.broadcastEvent( clone );
  }
}


static Boolean _matchIdent( struct OOperator* inst, const char* ident1, const char* ident2, const char* ident3, const char* ident4 ) {
  iOOperatorData data = Data(inst);
  Boolean match = False;
  iOStrTok tok = StrTokOp.inst(wOperator.getcarids(data->props), ',');
  while( StrTokOp.hasMoreTokens(tok) ) {
    iOCar car = ModelOp.getCar(AppOp.getModel(), StrTokOp.nextToken(tok) );
    if( car != NULL ) {
      if( ident1 != NULL && StrOp.len(ident1) > 0 && StrOp.equals(ident1, CarOp.getIdent(car) ) ) {
        match = True;
        break;
      }
      else if( ident2 != NULL && StrOp.len(ident2) > 0 && StrOp.equals(ident2, CarOp.getIdent(car) ) ) {
        match = True;
        break;
      }
      else if( ident3 != NULL && StrOp.len(ident3) > 0 && StrOp.equals(ident3, CarOp.getIdent(car) ) ) {
        match = True;
        break;
      }
      else if( ident4 != NULL && StrOp.len(ident4) > 0 && StrOp.equals(ident4, CarOp.getIdent(car) ) ) {
        match = True;
        break;
      }

    }
  }
  StrTokOp.base.del(tok);

  return match;
}


static int _getLen( struct OOperator* inst, int* trainweight ) {
  iOOperatorData data = Data(inst);
  /* ToDo: Calculate consist length. */
  int len = 0;
  int weight = 0;
  iOStrTok tok = StrTokOp.inst(wOperator.getcarids(data->props), ',');
  while( StrTokOp.hasMoreTokens(tok) ) {
    iOCar car = ModelOp.getCar(AppOp.getModel(), StrTokOp.nextToken(tok) );
    if( car != NULL ) {
      len += CarOp.getLen(car);
      weight += CarOp.getWeight(car);
    }
  }
  StrTokOp.base.del(tok);
  if( trainweight != NULL )
    *trainweight = weight;
  return len;
}


/* ----- DO NOT REMOVE OR EDIT THIS INCLUDE LINE! -----*/
#include "rocrail/impl/operator.fm"
/* ----- DO NOT REMOVE OR EDIT THIS INCLUDE LINE! -----*/
