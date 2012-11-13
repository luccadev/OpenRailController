/*
 Rocrail - Model Railroad Software

 Copyright (C) 2002-2012 Rob Versluis <r.j.versluis@rocrail.net>, Jean-Michel Fischer <jmf@polygonpunkt.de>

 Without an official permission commercial use is not permitted.
 Forking this project is not permitted.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

/*
=Track Plan Analyser=

==Preface==
Goal of the analyser is to find routes between blocks by checking all surrounded objects.

==Preparation==
create object map with xyz key's

==Processing==
Iterate the block list and try to find routes to other blocks by scanning the surrounded objects by xyz key's
and take in account the object orientations.

==Example==

--[block A]--+--[block B]--
--[block C]--/

Routes are A-B and B-C.
xyz = 0,0,0 in the left upper corner of the track plan.

**object map:**
 x,y,z object type     orientation  
 0,0,0 track  straight W
 1,0,0 track  straight W
11,0,0 track  straight W
12,0,0 track  straight W
13,0,0 switch left     W
14,0,0 track  straight W
15,0,0 track  straight W
25,0,0 track  straight W
25,0,0 track  straight W
 0,1,0 track  straight W
 1,1,0 track  straight W
11,1,0 track  straight W
12,1,0 track  straight W
13,1,0 track  curve    S

**block list:**
 2,0,0 A W
16,0,0 B W
 2,1,0 C W

N = north
E = east
S = south
W = west

For an example on how objects orientations work out see Rocrail/doc/analyse.xml

Starting with block A:
- block raster length for this example is 9
- only at the west and east side could be connected objects: search in map for 1,0,0 and 11,0,0
- following the west side will end up to a dead end.
- the east side will lead to block B following the tracks and switch

For the Analyzer to work the Plan has to fullfill:
- all items must be connected without space
- only one item at one position
 */

#include "rocrail/impl/analyse_impl.h"

#include "rocs/public/mem.h"
#include "rocs/public/trace.h"
#include "rocs/public/strtok.h"

#include "rocrail/wrapper/public/Block.h"
#include "rocrail/wrapper/public/Item.h"
#include "rocrail/wrapper/public/Track.h"
#include "rocrail/wrapper/public/Switch.h"
#include "rocrail/wrapper/public/Feedback.h"
#include "rocrail/wrapper/public/ModPlan.h"
#include "rocrail/wrapper/public/Module.h"
#include "rocrail/wrapper/public/Route.h"
#include "rocrail/wrapper/public/RouteList.h"
#include "rocrail/wrapper/public/Plan.h"
#include "rocrail/wrapper/public/Signal.h"
#include "rocrail/wrapper/public/SelTab.h"
#include "rocrail/wrapper/public/Stage.h"
#include "rocrail/wrapper/public/FeedbackEvent.h"
#include "rocrail/wrapper/public/Ctrl.h"
#include "rocrail/wrapper/public/RocRail.h"
#include "rocrail/wrapper/public/Action.h"
#include "rocrail/wrapper/public/ActionCtrl.h"
#include "rocrail/wrapper/public/Location.h"
#include "rocrail/wrapper/public/Loc.h"
#include "rocrail/wrapper/public/Output.h"


#include "rocrail/public/app.h"
#include "rocrail/public/model.h"
#include "rocrail/public/track.h"
#include "rocrail/public/switch.h"
#include "rocrail/public/signal.h"
#include "rocrail/public/fback.h"
#include "rocrail/public/route.h"


static int instCnt = 0;

/** ----- OBase ----- */
static const char* __id( void* inst ) {
  return NULL;
}

static void* __event( void* inst, const void* evt ) {
  return NULL;
}

static void __del( void* inst ) {
  if( inst != NULL ) {
    iOAnalyseData data = Data(inst);
    TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "cleaning up the ANALYSER...");

    iOMap delMap = MapOp.inst();
    char delkey[32];

    iOList plist = (iOList) ListOp.first(data->prelist);
    while (plist != NULL) {
      iONode item = (iONode) ListOp.first(plist);
      while (item != NULL) {
        if( item != NULL ) {
          StrOp.fmtb(delkey, "0x%08X", (void*)item);
          if(!MapOp.haskey(delMap, delkey)) {
            TraceOp.trc( name, TRCLEVEL_DEBUG, __LINE__, 9999, "delete %s[0x%08X]", ((obj) item)->name(), (void*)item);
            MapOp.put(delMap, delkey, (obj)delkey);
            NodeOp.base.del(item);
          }
        }
        item = (iONode) ListOp.next(plist);
      }
      StrOp.fmtb(delkey, "0x%08X", (void*)item);
      if(!MapOp.haskey(delMap, delkey)) {
        TraceOp.trc( name, TRCLEVEL_DEBUG, __LINE__, 9999, "delete %s[0x%08X]", ((obj) plist)->name(), (void*)plist);
        MapOp.put(delMap, delkey, (obj)delkey);
        ListOp.base.del(plist);
      }
      plist = (iOList) ListOp.next(data->prelist);
    }

    iONode item = (iONode) ListOp.first(data->bklist);
    while (item != NULL) {
      StrOp.fmtb(delkey, "0x%08X", (void*)item);
      if(!MapOp.haskey(delMap, delkey)) {
        TraceOp.trc( name, TRCLEVEL_DEBUG, __LINE__, 9999, "delete %s[0x%08X]", ((obj) item)->name(), (void*)item);
        MapOp.put(delMap, delkey, (obj)delkey);
        NodeOp.base.del(item);
      }
      item = (iONode) ListOp.next(data->bklist);
    }

    iOList occlist = (iOList) ListOp.first(data->bkoccitemlist);
    while (occlist) {
      iONode item = (iONode) ListOp.first(occlist);
      while (item) {
        StrOp.fmtb(delkey, "0x%08X", (void*)item);
        if(!MapOp.haskey(delMap, delkey)) {
          TraceOp.trc( name, TRCLEVEL_DEBUG, __LINE__, 9999, "delete %s[0x%08X]", ((obj) item)->name(), (void*)item);
          MapOp.put(delMap, delkey, (obj)delkey);
          NodeOp.base.del(item);
        }
        item = (iONode) ListOp.next(occlist);
      }
      StrOp.fmtb(delkey, "0x%08X", (void*)item);
      if(!MapOp.haskey(delMap, delkey)) {
        TraceOp.trc( name, TRCLEVEL_DEBUG, __LINE__, 9999, "delete %s[0x%08X]", ((obj) occlist)->name(), (void*)occlist);
        MapOp.put(delMap, delkey, (obj)delkey);
        ListOp.base.del(occlist);
      }
      occlist = (iOList) ListOp.next(data->bkoccitemlist);
    }

    MapOp.base.del(data->objectmap);
    ListOp.base.del(data->bklist);
    ListOp.base.del(data->prelist);
    ListOp.base.del(data->bkoccitemlist);

    MapOp.base.del(delMap);

    TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "ANALYSER is cleaned up");

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
  return NULL;
}

/** ----- OAnalyse ----- */
static const int BlockCX = 4;

#define oriWest  0
#define oriNorth 1
#define oriEast  2
#define oriSouth 3

#define oriWest2  4
#define oriNorth2 5
#define oriEast2  6
#define oriSouth2 7

static const Boolean analyserStrict = True;
static Boolean cleanrun = False; /* Will clean all autogenroutes and all route representation*/

static void __analyseBlock(iOAnalyse inst, iONode block, const char * inittravel);
static Boolean __analyseItem(iOAnalyse inst, iONode item, iOList route, iOList occ, int travel,
    int turnoutstate, int depth, int searchingSignal, Boolean behindABlock);
static Boolean _checkPlanHealth(iONode model);

/* returns 0 for west, 1 for north, 2 for east and 3 for south */
static int __getOri(iONode item ) {
  const char* ori = wItem.getori(item);
  if( ori == NULL ) return oriWest;
  if( StrOp.equals( wItem.west , ori ) ) return oriWest;
  if( StrOp.equals( wItem.north, ori ) ) return oriNorth;
  if( StrOp.equals( wItem.east , ori ) ) return oriEast;
  if( StrOp.equals( wItem.south, ori ) ) return oriSouth;
  return oriWest;
}

static char* __createKey( char* key, iONode node, int xoffset, int yoffset, int zoffset) {

  int itemx = 0;
  int itemy = 0;
  if( node != NULL) {
    itemx = wItem.getx(node);
    itemy = wItem.gety(node);
  }
  return StrOp.fmtb( key, "%d-%d-%d", itemx+xoffset, itemy+yoffset, wItem.getz(node) );
}

static void __prepare(iOAnalyse inst, iOList list, int modx, int mody, Boolean modplan) {
  iOAnalyseData data = Data(inst);
  char key[32] = {'\0'};


  iONode nodetmp = (iONode)ListOp.first( list );
  iONode node = NULL;

  /* clone the node */
  if( nodetmp != NULL)
    node = (iONode)NodeOp.base.clone( nodetmp);

  TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "mod x: %d y: %d ",
            modx, mody );

  while( node != NULL ) {
    if( StrOp.equals( wBlock.name(), NodeOp.getName(node) ) ||
        StrOp.equals( wSelTab.name(), NodeOp.getName(node) ) ||
        StrOp.equals( wStage.name(), NodeOp.getName(node) ))
    {
      TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, " ------>  block %s with key (old) %s",
          wBlock.getid(node), __createKey( key, node, 0+modx, 0+mody, 0) );

      ListOp.add( data->bklist, (obj)node );
    }
    { /*blocks as well in the map!*/
      /* put the object in the map */

      const char * ori = wItem.getori(node);
      if( ori == NULL ) {
        ori = "west";
      }

      if( modplan)
        wItem.setz( node, 0);

      const char* type = wItem.gettype(node);
      __createKey( key, node, 0+modx, 0+mody, 0);
      TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "  adding key %s for %s type: %s ori: %s name: %s",
          key, NodeOp.getName(node), type==NULL?"":type, ori, wItem.getid(node) );
      MapOp.put( data->objectmap, key, (obj)node);

      wItem.setx( node, wItem.getx(node)+modx);
      wItem.sety( node, wItem.gety(node)+mody);

      /* put keys for all covered fields */
      if( StrOp.equals( NodeOp.getName(node), "sw" ) ) {
        if( StrOp.equals( wItem.gettype(node), "crossing" ) ||
            StrOp.equals( wItem.gettype(node), "dcrossing" ) ||
            StrOp.equals( wItem.gettype(node), "ccrossing" )) {

          if( StrOp.equals( ori, "east" ) || StrOp.equals( ori, "west" ) ) {
            __createKey( key, node, 1, 0, 0);
            TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "  adding key %s for %s type: %s ori: %s name: %s",
                      key, NodeOp.getName(node), type==NULL?"":type, wItem.getori(node), ori );

            MapOp.put( data->objectmap, key, (obj)node);
          }
          if( StrOp.equals( ori, "north" ) || StrOp.equals( ori, "south" ) ) {
            __createKey( key, node, 0, 1, 0);
            TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "  adding key %s for %s type: %s ori: %s name: %s",
                      key, NodeOp.getName(node), type==NULL?"":type, wItem.getori(node), ori );

            MapOp.put( data->objectmap, key, (obj)node);
          }
        }
      }

      if( StrOp.equals( NodeOp.getName(node), "bk" ) ) {

        int fields = 4;
        if( wBlock.issmallsymbol(node) ) {
          fields = 2;
        }

        int i;
        for (i=1;i<fields;i++) {

          if( StrOp.equals( ori, "east" ) || StrOp.equals( ori, "west" ) ) {
            __createKey( key, node, i, 0, 0);
            TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "  adding key %s for %s type: %s ori: %s name: %s",
                      key, NodeOp.getName(node), type==NULL?"":type, ori, wItem.getid(node) );

            MapOp.put( data->objectmap, key, (obj)node);
          }
          if( StrOp.equals( ori, "north" ) || StrOp.equals( ori, "south" ) ) {
            __createKey( key, node, 0, i, 0);
            TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "  adding key %s for %s type: %s ori: %s name: %s",
                      key, NodeOp.getName(node), type==NULL?"":type, ori, wItem.getid(node) );

            MapOp.put( data->objectmap, key, (obj)node);
          }
        }
      }

      if( StrOp.equals( NodeOp.getName(node), "seltab" ) ) {

        int fields = wSelTab.getnrtracks( node);

        TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "  SELTAB %d",
            fields);

        int i;
        for (i=1;i<fields;i++) {

          if( StrOp.equals( ori, "east" ) || StrOp.equals( ori, "west" ) ) {
            __createKey( key, node, i, 0, 0);
            TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "  adding key %s for %s type: %s ori: %s name: %s",
                      key, NodeOp.getName(node), type==NULL?"":type, ori, wItem.getid(node) );

            MapOp.put( data->objectmap, key, (obj)node);
          }
          if( StrOp.equals( ori, "north" ) || StrOp.equals( ori, "south" ) ) {
            __createKey( key, node, 0, i, 0);
            TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "  adding key %s for %s type: %s ori: %s name: %s",
                      key, NodeOp.getName(node), type==NULL?"":type, ori, wItem.getid(node) );

            MapOp.put( data->objectmap, key, (obj)node);
          }
        }
      }

    }

    iONode nextnode = (iONode)ListOp.next( list );
    if( nextnode != NULL)
      node = (iONode)NodeOp.base.clone( nextnode);
    else
      node = NULL;
  };
  /*return bklist;*/
}


static const int typeTrackStraight  = 0;
static const int typeTrackCurve  = 1;
static const int typeBlock  = 2;
static const int typeSwitch  = 3;
static const int itemNotInDirection = 1000;

static int __getType(iONode item ) {
  const char* type = NodeOp.getName(item);
  const char* subtype = wItem.gettype(item);

  /*if( StrOp.equals( "tk" , type ) || StrOp.equals( wFeedback.name() , type ) ) {*/
    if(        StrOp.equals( wTrack.curve, subtype ) ) {

      return typeTrackCurve;

    } else if( StrOp.equals( "sw", type ) ) {
      return typeSwitch;
    } else {
      return typeTrackStraight;
    }

  TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "  --- GETTYPE: FAILED should not happen");
  return -1;
}


static const int foundBlock = 100;
static const int twoWayTurnout = 200;
static const int threeWayTurnout = 300;
static const int dcrossing = 400;
static const int dcrossingAhead = 2000;

static int __travel( iONode item, int travel, int turnoutstate, int * turnoutstate_out, int * x, int * y, const char * key) {
  if( item ) {
    const char * itemori = wItem.getori(item);

    const char* type = NodeOp.getName(item);
    const char* subtype = wItem.gettype(item);

    int mori = __getOri(item);

    *turnoutstate_out = 0;
    *x = 0;
    *y = 0;

    /* missing default values */
    if( itemori == NULL) {
      itemori = "west";
      /*TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "set default");*/
    }

    /* curve -> change dir */
      if( __getType(item) == typeTrackCurve  || (StrOp.equals( type , "fb" ) && wFeedback.iscurve( item))   ) {
        /* algebra of Rocrail directions */
        if(        travel == 0 &&  StrOp.equals( itemori, "north" )) {
          return oriSouth;
        } else if( travel == 0 &&  StrOp.equals( itemori, "east" )) {
          return oriNorth;
        } else if( travel == 1 &&  StrOp.equals( itemori, "west" )) {
          return oriWest;
        } else if( travel == 1 &&  StrOp.equals( itemori, "north" )) {
          return oriEast;
        } else if( travel == 2 &&  StrOp.equals( itemori, "west" )) {
          return oriSouth;
        } else if( travel == 2 &&  StrOp.equals( itemori, "south" )) {
          return oriNorth;
        } else if( travel == 3 &&  StrOp.equals( itemori, "east" )) {
          return oriEast;
        } else if( travel == 3 &&  StrOp.equals( itemori, "south" )) {
          return oriWest;
        } else {
          TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "  -- no valid curve for us! [%s]",
              wItem.getid(item) );
          return -1; /*end of the game */
        }
      } else if (StrOp.equals( "tk" , type ) && StrOp.equals( subtype , "dir" ) ) {

        if( travel == 1 && mori == 3 )
          return travel;
        else if( travel == 3 && mori == 1 )
          return travel;
        else if( travel == 0 && mori == 0 )
          return travel;
        else if( travel == 2 && mori == 2 )
          return travel;
        return itemNotInDirection;
      }
      /* block */
      else if( StrOp.equals( NodeOp.getName(item) , "bk" )) {

        int step = 3;
        if( wBlock.issmallsymbol( item )) {
          step = 1;
        }

        if( StrOp.equals( itemori, "west" ) || StrOp.equals( itemori, "east" )) {
          if( (travel == 2) ) {
            *x = step;
            return travel;
          } else if ( travel == 0)
            return travel;
        }
        else if( StrOp.equals( itemori, "north" ) || StrOp.equals( itemori, "south" )) {
          if( (travel == 3) ) {
            *y = step;
            return travel;
          } else if ( travel == 1)
            return travel;
        }
        return itemNotInDirection;
      }

      /* block */
      else if( StrOp.equals( NodeOp.getName(item) , "seltab" )) {

        int step = wSelTab.getnrtracks( item);

        if( StrOp.equals( itemori, "west" ) || StrOp.equals( itemori, "east" )) {
          if( (travel == 2) ) {
            *x = step;
            return travel;
          } else if ( travel == 0)
            return travel;
        }
        else if( StrOp.equals( itemori, "north" ) || StrOp.equals( itemori, "south" )) {
          if( (travel == 3) ) {
            *y = step;
            return travel;
          } else if ( travel == 1)
            return travel;
        }
        return itemNotInDirection;
      }
      /* decoupler */
      else if( __getType(item) == typeSwitch && StrOp.equals( subtype, "decoupler")) {
        return travel;
      }
      /* switch */
      else if( __getType(item) == typeSwitch) {
        /* coming from the points */
        if(        travel == 0 &&  StrOp.equals( itemori, "east" )
                && StrOp.equals( wItem.gettype(item), "right" ) ) {

          if(turnoutstate == 1)
            return oriNorth+twoWayTurnout;

          return travel+twoWayTurnout;
        } else if( travel == 0 &&  StrOp.equals( itemori, "west" )
                && StrOp.equals( wItem.gettype(item), "left" ) ) {

          if(turnoutstate == 1)
            return oriSouth+twoWayTurnout;

          return travel+twoWayTurnout;
        } else if( travel == 1 &&  StrOp.equals( itemori, "north" )
                && StrOp.equals( wItem.gettype(item), "right" ) ) {

          if(turnoutstate == 1)
            return oriEast+twoWayTurnout;

          return travel+twoWayTurnout;
        } else if( travel == 1 &&  StrOp.equals( itemori, "south" )
                && StrOp.equals( wItem.gettype(item), "left" ) ) {

          if(turnoutstate == 1)
            return oriWest+twoWayTurnout;

          return travel+twoWayTurnout;
        } else if( travel == 2 &&  StrOp.equals( itemori, "west" )
                && StrOp.equals( wItem.gettype(item), "right" ) ) {

          if(turnoutstate == 1)
            return oriSouth+twoWayTurnout;

          return travel+twoWayTurnout;
        } else if( travel == 2 &&  StrOp.equals( itemori, "east" )
                && StrOp.equals( wItem.gettype(item), "left" ) ) {

          if(turnoutstate == 1)
            return oriNorth+twoWayTurnout;

          return travel+twoWayTurnout;
        } else if( travel == 3 &&  StrOp.equals( itemori, "south" )
                && StrOp.equals( wItem.gettype(item), "right" ) ) {

          if(turnoutstate == 1)
            return oriWest+twoWayTurnout;

          return travel+twoWayTurnout;
        } else if( travel == 3 &&  StrOp.equals( itemori, "north" )
                && StrOp.equals( wItem.gettype(item), "left" ) ) {

          if(turnoutstate == 1)
            return oriEast+twoWayTurnout;

          return travel+twoWayTurnout;
        }

        /* coming from the frog -> straight line */
        else if( travel == 0 &&  StrOp.equals( itemori, "west" )
                && StrOp.equals( wItem.gettype(item), "right" ) ) {
          *turnoutstate_out = 0;
          return travel;
        } else if( travel == 0 &&  StrOp.equals( itemori, "east" )
                && StrOp.equals( wItem.gettype(item), "left" ) ) {
          *turnoutstate_out = 0;
          return travel;
        } else if( travel == 1 &&  StrOp.equals( itemori, "south" )
                && StrOp.equals( wItem.gettype(item), "right" ) ) {
          *turnoutstate_out = 0;
          return travel;
        } else if( travel == 1 &&  StrOp.equals( itemori, "north" )
                && StrOp.equals( wItem.gettype(item), "left" ) ) {
          *turnoutstate_out = 0;
          return travel;
        } else if( travel == 2 &&  StrOp.equals( itemori, "east" )
               && StrOp.equals( wItem.gettype(item), "right" ) ) {
          *turnoutstate_out = 0;
          return travel;
        } else if( travel == 2 &&  StrOp.equals( itemori, "west" )
               && StrOp.equals( wItem.gettype(item), "left" ) ) {
          *turnoutstate_out = 0;
          return travel;
        } else if( travel == 3 &&  StrOp.equals( itemori, "north" )
              && StrOp.equals( wItem.gettype(item), "right" ) ) {
          *turnoutstate_out = 0;
          return travel;
        } else if( travel == 3 &&  StrOp.equals( itemori, "south" )
              && StrOp.equals( wItem.gettype(item), "left" ) ) {
          *turnoutstate_out = 0;
          return travel;
        }

        /* coming from the frog -> diverging line */
        else if( travel == 0 &&  StrOp.equals( itemori, "north" )
                && StrOp.equals( wItem.gettype(item), "right" ) ) {
          *turnoutstate_out = 1;
          return oriSouth;
        } else if( travel == 0 &&  StrOp.equals( itemori, "north" )
                && StrOp.equals( wItem.gettype(item), "left" ) ) {
          *turnoutstate_out = 1;
          return oriNorth;
        } else if( travel == 1 &&  StrOp.equals( itemori, "west" )
                && StrOp.equals( wItem.gettype(item), "right" ) ) {
          *turnoutstate_out = 1;
          return oriWest;
        } else if( travel == 1 &&  StrOp.equals( itemori, "west" )
                && StrOp.equals( wItem.gettype(item), "left" ) ) {
          *turnoutstate_out = 1;
          return oriEast;
        } else if( travel == 2 &&  StrOp.equals( itemori, "south" )
               && StrOp.equals( wItem.gettype(item), "right" ) ) {
          *turnoutstate_out = 1;
          return oriNorth;
        } else if( travel == 2 &&  StrOp.equals( itemori, "south" )
               && StrOp.equals( wItem.gettype(item), "left" ) ) {
          *turnoutstate_out = 1;
          return oriSouth;
        } else if( travel == 3 &&  StrOp.equals( itemori, "east" )
              && StrOp.equals( wItem.gettype(item), "right" ) ) {
          *turnoutstate_out = 1;
          return oriEast;
        } else if( travel == 3 &&  StrOp.equals( itemori, "east" )
              && StrOp.equals( wItem.gettype(item), "left" ) ) {
          *turnoutstate_out = 1;
          return oriWest;
        }

        /* crossing */
        else if( StrOp.equals( wItem.gettype(item), "crossing" ) && wSwitch.getaddr1(item) == 0 && wSwitch.getport1(item) == 0 ) {
          TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, " crossing %d travel: %d", wSwitch.isdir(item), travel );

          /* rectcrossing */
          if( wSwitch.isrectcrossing(item)){
            TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, " rectcrossing");
            return travel;
          }

          if( !wSwitch.isdir(item)  ) { /* left */
            if( StrOp.equals( itemori, "west" ) || StrOp.equals( itemori, "east" )) {
              if( (travel == 1) || (travel == 2)) {
                *x = 1;
                return travel;
              }
              return travel;
            }
            else if( StrOp.equals( itemori, "north" ) || StrOp.equals( itemori, "south" )) {
              if ( (travel == 2) || (travel == 3) ) {
                *y = 1;
                return travel;
              }
              return travel;
            }
          } else if( wSwitch.isdir(item) ) { /* right */
            if( StrOp.equals( itemori, "west" ) || StrOp.equals( itemori, "east" )) {
              if ( (travel == 3) || (travel == 2) ) {
                *x = 1;
                return travel;
              }
              return travel;
            }
            else if( StrOp.equals( itemori, "north" ) || StrOp.equals( itemori, "south" )) {
              if( (travel == 0) || (travel == 3) ) {
                *y = 1;
                return travel;
              }
              return travel;
            }
          }
          return itemNotInDirection;
        }
        /* ccrossing */
        else if( StrOp.equals( wItem.gettype(item), "ccrossing" ) ) {

          char mkey[32] = {'\0'};
          __createKey( mkey, item, 0, 0, 0);

          int sign = 1;
          if( !StrOp.equals( key, mkey ) ) {
            sign = -1;
          }

          if( StrOp.equals( itemori, "west" ) || StrOp.equals( itemori, "east" )) {
            if( (travel == 1) ) {
              *x = sign;
              return travel;
            } else if ( travel == 3 ) {
              *x = sign;
              return travel;
            }
          }
          else if( StrOp.equals( itemori, "north" ) || StrOp.equals( itemori, "south" )) {
            if( (travel == 0) ) {
              *y = sign;
              return travel;
            } else if ( travel == 2 ) {
              *y = sign;
              return travel;
            }
          }
        }
        /* dcrossing */
        else if( StrOp.equals( wItem.gettype(item), "dcrossing" ) ||
                 StrOp.equals( wItem.gettype(item), "crossing" ) && (wSwitch.getaddr1(item) != 0 || wSwitch.getport1(item) != 0 ) )
        {
          /*TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, " +++++++%d+itemori:[%s]+++++++ dcrossing travel: %d turnoutstate: %d",
              wSwitch.isdir(item),itemori,travel, turnoutstate );*/

          if( !wSwitch.isdir(item)  ) { /* left */
            if( StrOp.equals( itemori, "west" ) ) {
              if( (travel == 0) ) {
                if ( turnoutstate == 0) {
                  return travel+dcrossing;
                } else if( turnoutstate == 2) {
                  return oriSouth+dcrossing;
                }
              } else if ( travel == 1 ) {
                if ( turnoutstate == 1) {
                  *x = 1;
                  return travel+dcrossing;
                } else if( turnoutstate == 2) {
                  *x = 1;
                  return oriEast+dcrossing;
                }
              } else if ( travel == 2 ) {
                if ( turnoutstate == 0) {
                  *x = 1;
                  return travel+dcrossing;
                } else if( turnoutstate == 3) {
                  *x = 1;
                  return oriNorth+dcrossing;
                }
              } else if ( travel == 3 ) {
                if ( turnoutstate == 1) {
                  return travel+dcrossing;
                } else if( turnoutstate == 3) {
                  return oriWest+dcrossing;
                }
              }
            }
            if( StrOp.equals( itemori, "east" ) ) {
              if( (travel == 0) ) {
                if ( turnoutstate == 0) {
                  return travel+dcrossing;
                } else if( turnoutstate == 3) {
                  return oriSouth+dcrossing;
                }
              } else if ( travel == 1 ) {
                if ( turnoutstate == 1) {
                  *x = 1;
                  return travel+dcrossing;
                } else if( turnoutstate == 3) {
                  *x = 1;
                  return oriEast+dcrossing;
                }
              } else if ( travel == 2 ) {
                if ( turnoutstate == 0) {
                  *x = 1;
                  return travel+dcrossing;
                } else if( turnoutstate == 2) {
                  *x = 1;
                  return oriNorth+dcrossing;
                }
              } else if ( travel == 3 ) {
                if ( turnoutstate == 1) {
                  return travel+dcrossing;
                } else if( turnoutstate == 2) {
                  return oriWest+dcrossing;
                }
              }
            }
            else if( StrOp.equals( itemori, "north" )) { /* left north */
              if( (travel == 0) ) {
                if ( turnoutstate == 1) {
                  return travel+dcrossing;
                } else if( turnoutstate == 2) {
                  return oriNorth+dcrossing;
                }
              } else if ( travel == 1 ) {
                if ( turnoutstate == 0) {
                  return travel+dcrossing;
                } else if( turnoutstate == 3) {
                  return oriWest+dcrossing;
                }
              } else if ( travel == 2 ) {
                if ( turnoutstate == 1) {
                  *y = 1;
                  return travel+dcrossing;
                } else if( turnoutstate == 3) {
                  *y = 1;
                  return oriSouth+dcrossing;
                }
              } else if ( travel == 3 ) {
                if ( turnoutstate == 0) {
                  *y = 1;
                  return travel+dcrossing;
                } else if( turnoutstate == 2) {
                  *y = 1;
                  return oriEast+dcrossing;
                }
              }
            }
            else if( StrOp.equals( itemori, "south" )) {
              if( (travel == 0) ) {
                if ( turnoutstate == 1) {
                  return travel+dcrossing;
                } else if( turnoutstate == 3) {
                  return oriNorth+dcrossing;
                }
              } else if ( travel == 1 ) {
                if ( turnoutstate == 0) {
                  return travel+dcrossing;
                } else if( turnoutstate == 2) {
                  return oriWest+dcrossing;
                }
              } else if ( travel == 2 ) {
                if ( turnoutstate == 1) {
                  *y = 1;
                  return travel+dcrossing;
                } else if( turnoutstate == 2) {
                  *y = 1;
                  return oriSouth+dcrossing;
                }
              } else if ( travel == 3 ) {
                if ( turnoutstate == 0) {
                  *y = 1;
                  return travel+dcrossing;
                } else if( turnoutstate == 3) {
                  *y = 1;
                  return oriEast+dcrossing;
                }
              }
            }
          } else if( wSwitch.isdir(item) ) { /* right */
            if( StrOp.equals( itemori, "west" )) {
              if( (travel == 0) ) {
                if ( turnoutstate == 0) {
                  return travel+dcrossing;
                } else if( turnoutstate == 3) {
                  return oriNorth+dcrossing;
                }
              } else if ( travel == 1 ) {
                if ( turnoutstate == 1) {
                  return travel+dcrossing;
                } else if( turnoutstate == 2) {
                  return oriWest+dcrossing;
                }
              } else if ( travel == 2 ) {
                if ( turnoutstate == 0) {
                  *x = 1;
                  return travel+dcrossing;
                } else if( turnoutstate == 2) {
                  *x = 1;
                  return oriSouth+dcrossing;
                }
              } else if ( travel == 3 ) {
                if ( turnoutstate == 1) {
                  *x = 1;
                  return travel+dcrossing;
                } else if( turnoutstate == 3) {
                  *x = 1;
                  return oriEast+dcrossing;
                }
              }
            }
              if( StrOp.equals( itemori, "east" )) { /* right east */
               if( (travel == 0) ) {
                 if ( turnoutstate == 0) {
                   return travel+dcrossing;
                 } else if( turnoutstate == 2) {
                   return oriNorth+dcrossing;
                 }
               } else if ( travel == 1 ) {
                 if ( turnoutstate == 1) {
                   return travel+dcrossing;
                 } else if( turnoutstate == 3) {
                   return oriWest+dcrossing;
                 }
               } else if ( travel == 2 ) {
                 if ( turnoutstate == 0) {
                   *x = 1;
                   return travel+dcrossing;
                 } else if( turnoutstate == 3) {
                   *x = 1;
                   return oriSouth+dcrossing;
                 }
               } else if ( travel == 3 ) {
                 if ( turnoutstate == 1) {
                   *x = 1;
                   return travel+dcrossing;
                 } else if( turnoutstate == 2) {
                   *x = 1;
                   return oriEast+dcrossing;
                 }
               }
            }
              else if( StrOp.equals( itemori, "north" )) { /* right north */
              if( (travel == 0) ) {
                if ( turnoutstate == 1) {
                  *y = 1;
                  return travel+dcrossing;
                } else if( turnoutstate == 2) {
                  *y = 1;
                  return oriSouth+dcrossing;
                }
              } else if ( travel == 1 ) {
                if ( turnoutstate == 0) {
                  return travel+dcrossing;
                } else if( turnoutstate == 2) {
                  return oriEast+dcrossing;
                }
              } else if ( travel == 2 ) {
                if ( turnoutstate == 1) {
                  return travel+dcrossing;
                } else if( turnoutstate == 3) {
                  return oriNorth+dcrossing;
                }
              } else if ( travel == 3 ) {
                if ( turnoutstate == 0) {
                  *y = 1;
                  return travel+dcrossing;
                } else if( turnoutstate == 3) {
                  *y = 1;
                  return oriWest+dcrossing;
                }
              }
            }
            else if( StrOp.equals( itemori, "south" )) { /* right south */
              if( (travel == 0) ) {
                if ( turnoutstate == 1) {
                  *y = 1;
                  return travel+dcrossing;
                } else if( turnoutstate == 3) {
                  *y = 1;
                  return oriSouth+dcrossing;
                }
              } else if ( travel == 1 ) {
                if ( turnoutstate == 0) {
                  return travel+dcrossing;
                } else if( turnoutstate == 3) {
                  return oriEast+dcrossing;
                }
              } else if ( travel == 2 ) {
                if ( turnoutstate == 1) {
                  return travel+dcrossing;
                } else if( turnoutstate == 2) {
                  return oriNorth+dcrossing;
                }
              } else if ( travel == 3 ) {
                if ( turnoutstate == 0) {
                  *y = 1;
                  return travel+dcrossing;
                } else if( turnoutstate == 2) {
                  *y = 1;
                  return oriWest+dcrossing;
                }
              }
            }
          }
          TraceOp.trc( name, TRCLEVEL_DEBUG, __LINE__, 9999, "  "
              "DCROSSING travel:%d- turnoutstate: %d itemori: %s id: %s dir: %d",
              travel,turnoutstate, itemori,wItem.getid(item),wSwitch.isdir(item));
          return dcrossingAhead + travel;
        }
        /* threeway */
        else if( StrOp.equals( wItem.gettype(item), "threeway" ) ) {

          if( StrOp.equals( itemori, "west" )) {
            if( (travel == 0) ) {
              if( turnoutstate == 0) { /* center */
                return travel+threeWayTurnout;
              } else if (turnoutstate == 1) { /* left */
                return oriSouth+threeWayTurnout;
              } else if (turnoutstate == 2) { /* right */
                return oriNorth+threeWayTurnout;
              }
            }
            else if ( travel == 1 ) {
              *turnoutstate_out = 1;
              return oriEast;
            }
            else if ( travel == 2 ) {
              *turnoutstate_out = 0;
              return travel;
            }
            else if ( travel == 3 ) {
              *turnoutstate_out = 2;
              return oriEast;
            }
          }
          else if( StrOp.equals( itemori, "north" )) {
            if( (travel == 0) ) {
              *turnoutstate_out = 1;
              return oriNorth;
            }
            else if ( travel == 1 ) {
              *turnoutstate_out = 0;
              return travel;
            }
            else if ( travel == 2 ) {
              *turnoutstate_out = 2;
              return oriNorth;
            }
            else if ( travel == 3 ) {
              if( turnoutstate == 0) {
                return travel+threeWayTurnout;
              } else if (turnoutstate == 1) {
                return oriEast+threeWayTurnout;
              } else if (turnoutstate == 2) {
                return oriWest+threeWayTurnout;
              }
            }
          }
          else if( StrOp.equals( itemori, "east" )) {
            if( (travel == 0) ) {
              *turnoutstate_out = 0;
              return travel;
            }
            else if ( travel == 1 ) {
              *turnoutstate_out = 2;
              return oriWest;
            }
            else if ( travel == 2 ) {
              if( turnoutstate == 0) {
                return travel+threeWayTurnout;
              } else if (turnoutstate == 1) {
                return oriNorth+threeWayTurnout;
              } else if (turnoutstate == 2) {
                return oriSouth+threeWayTurnout;
              }
            }
            else if ( travel == 3 ) {
              *turnoutstate_out = 1;
              return oriWest;
            }
          }
          else if( StrOp.equals( itemori, "south" )) {
            if( (travel == 0) ) {
              *turnoutstate_out = 2;
              return oriSouth;
            }
            else if ( travel == 1 ) {
              if( turnoutstate == 0) {
                return travel+threeWayTurnout;
              } else if (turnoutstate == 1) {
                return oriWest+threeWayTurnout;
              } else if (turnoutstate == 2) {
                return oriEast+threeWayTurnout;
              }
            }
            else if ( travel == 2 ) {
              *turnoutstate_out = 1;
              return oriSouth;
            }
            else if ( travel == 3 ) {
              *turnoutstate_out = 0;
              return travel;
            }
          }
        }

        /* turnout in wrong direction*/
        else {
          TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "  ---------- no valid turnout for us! [%s]",
                        wItem.getid(item) );
          return -1; /*end of the game*/
        }

      } else { /* elements which do not change travel direction */
        /* is the item in our direction? */
        if( !(((StrOp.equals( itemori, "north" ) || StrOp.equals( itemori, "south" ))
                    && (travel == 1 || travel == 3)) ||
            ((StrOp.equals( itemori, "east" ) || StrOp.equals( itemori, "west" ))
                    && (travel == 0 || travel == 2))) ) {
          TraceOp.trc( name, TRCLEVEL_DEBUG, __LINE__, 9999,
              "    Item [%s] is not in our travel direction giving up",
                    wItem.getid(item) );
          return itemNotInDirection; /*puh*/
        } else {
          return travel;
        }
      }
  } else {
    TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "ITEM==NULL");
  }

  return itemNotInDirection;
}

static Boolean __analyseBehindConnector(iOAnalyse inst, iONode item, iOList route, iOList occ, int travel,
    int turnoutstate, int depth, int searchingSignal, Boolean behindABlock) {
  iOAnalyseData data = Data(inst);

  wItem.getx(item);

  int xoffset = 0;
  int yoffset = 0;
  char key[32] = {'\0'};

  int i = 0;
  for ( i = 0; i < 10; i++) {

  switch(travel) {
     case oriWest:
       xoffset--;
       __createKey( key, item, xoffset, yoffset, 0);
        break;
     case oriNorth:
       yoffset--;
       __createKey( key, item, xoffset, yoffset, 0);
        break;
     case oriEast:
       xoffset++;
       __createKey( key, item, xoffset, yoffset, 0);
        break;
     case oriSouth:
       yoffset++;
       __createKey( key, item, xoffset, yoffset, 0);
        break;
     }

     iONode nextitem = (iONode)MapOp.get( data->objectmap, key);

     if( nextitem != NULL ) {
       if( StrOp.equals(NodeOp.getName(nextitem) , "tk" ) && StrOp.equals(wItem.gettype(nextitem) , "connector" )) {


         const char * nextitemori = NodeOp.getStr( nextitem, "ori", "west");
         Boolean found = False;
         if( StrOp.equals( nextitemori, "west" ) && travel == 0){
           found = True;
         } else if( StrOp.equals( nextitemori, "north" ) && travel == 3){
           found = True;
         } else if( StrOp.equals( nextitemori, "east" ) && travel == 2){
           found = True;
         } else if( StrOp.equals( nextitemori, "south" ) && travel == 1){
           found = True;
         }

         if (found ) {
           TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "found conterpart: [%s]", wItem.getid(nextitem));

           __analyseItem(inst, nextitem, route, occ, travel, 0, depth, searchingSignal, behindABlock );
         }

         return found;
       }
     }
  }
  return False;
}


static Boolean __analyseItem(iOAnalyse inst, iONode item, iOList route, iOList occ, int travel,
    int turnoutstate, int depth, int searchingSignal, Boolean behindABlock) {
  iOAnalyseData data = Data(inst);
  char key[32] = {'\0'};
  iONode nextitem = NULL;
  int blockleftcounter = 0;

  const char * itemori = wItem.getori(item);
  if( itemori == NULL) {
   itemori = "west";
  }

  TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "start analyzing item [%s] travel: [%d]"
        " sfs: [%d]",
        wItem.getid(item), travel, searchingSignal);

  const char * state = " ";
  if( StrOp.equals( NodeOp.getName(item) , "sw" ) ) {
    if ( StrOp.equals(wItem.gettype(item), "right" ) || StrOp.equals(wItem.gettype(item), "left" ) ) {
    state = turnoutstate?"turnout":"straight";
    } else if ( StrOp.equals(wItem.gettype(item), "dcrossing" )  ) {
      if( turnoutstate == 0) state = "straight";
      if( turnoutstate == 1) state = "turnout";
      if( turnoutstate == 2) state = "left";
      if( turnoutstate == 3) state = "right";
    } else if ( StrOp.equals(wItem.gettype(item), "crossing" ) && (wSwitch.getaddr1(item) != 0 || wSwitch.getport1(item) != 0 )  ) {
      if( turnoutstate == 0) state = "straight";
      if( turnoutstate == 1) state = "straight";
      if( turnoutstate == 2) state = "turnout";
      if( turnoutstate == 3) state = "turnout";
    } else if ( StrOp.equals(wItem.gettype(item), "threeway" )  ) {
      if( turnoutstate == 0) state = "straight";
      if( turnoutstate == 1) state = "left";
      if( turnoutstate == 2) state = "right";
    }
  } else if( StrOp.equals( NodeOp.getName(item) , "bk" ) || StrOp.equals( NodeOp.getName(item) , "seltab" )) {
    state = "-";
    if( StrOp.equals( itemori, "west" ) && travel == 0){
      state = "+";
    } else if( StrOp.equals( itemori, "north" ) && travel == 3){
      state = "+";
    } else if( StrOp.equals( itemori, "east" ) && travel == 2){
      state = "+";
    } else if( StrOp.equals( itemori, "south" ) && travel == 1){
      state = "+";
    }
  } else if( StrOp.equals(NodeOp.getName(item) , "sg" ) ) {
    /*is the signal in our direction ?*/
    if( StrOp.equals( itemori, "west" ) && travel == 0){
      state = "yes";
    } else if( StrOp.equals( itemori, "north" ) && travel == 3){
      state = "yes";
    } else if( StrOp.equals( itemori, "east" ) && travel == 2){
      state = "yes";
    } else if( StrOp.equals( itemori, "south" ) && travel == 1){
      state = "yes";
    }
  }

  if( (!StrOp.equals(NodeOp.getName(item) , "bk" ) && !StrOp.equals(NodeOp.getName(item) , "seltab" ) ) ||
      (depth == 0 && (StrOp.equals(NodeOp.getName(item) , "bk" ) || StrOp.equals(NodeOp.getName(item) , "seltab" )) ) ) {
    /* ADD TO LIST */
    iONode itemA = (iONode)NodeOp.base.clone( item);
    wItem.setstate(itemA, state);
    ListOp.add( route, (obj)itemA );
  }

  if( searchingSignal ) {
    iONode itemA = (iONode)NodeOp.base.clone( item);
    wItem.setstate(itemA, state);
    ListOp.add( occ, (obj)itemA );
  }

  if( StrOp.equals(NodeOp.getName(item) , "tk" )
      && StrOp.equals(wItem.gettype(item) , "connector" )) {

    Boolean found = False;
    if( StrOp.equals( itemori, "west" ) && travel == 2){
      found = True;
    } else if( StrOp.equals( itemori, "north" ) && travel == 1){
      found = True;
    } else if( StrOp.equals( itemori, "east" ) && travel == 0){
      found = True;
    } else if( StrOp.equals( itemori, "south" ) && travel == 3){
      found = True;
    }

    if( found) {
      TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "found connector: [%s] travel: [%d] ori: [%s]",
              wItem.getid(item) , travel, itemori);

      if( wTrack.getcounterpartid(item) != NULL && !StrOp.equals( wTrack.getcounterpartid(item) , "") ){
        TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "found counterpart for: [%s] counterpart: [%s]",
            wItem.getid(item),   wTrack.getcounterpartid(item) );

        iOTrack track = ModelOp.getTrack( data->model, wTrack.getcounterpartid(item) );

        /* go on at the connector */
        if( track != NULL) {
          iONode nextitem = TrackOp.base.properties(track);

          TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "continue at counterpart: [%s]",
                      wItem.getid(nextitem) );

          depth++;
          __analyseItem(inst, nextitem, route, occ, travel, turnoutstate, depth, searchingSignal, behindABlock);

        }

      } else {
        __analyseBehindConnector(inst, item, route, occ, travel, 0, depth, searchingSignal, behindABlock );
      }
    }

  }


  TraceOp.trc( name, TRCLEVEL_DEBUG, __LINE__, 9999, "depth: [%d]",
      depth );

  Boolean theEnd = False;
  /*security*/
  iONode listitem = (iONode)ListOp.first( occ );
  while(listitem) {
    if( !behindABlock
        && StrOp.equals(  wItem.getid(item),  wItem.getid(listitem) )
        && StrOp.equals(  NodeOp.getName(item),  NodeOp.getName(listitem) )
        && !StrOp.equals(NodeOp.getName(item) , "sw" )
        && !StrOp.equals(NodeOp.getName(item) , "bk" )) {
      TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "evil loop on [%s] [%s]",
          wItem.getid(item), NodeOp.getName(item));
      theEnd = True;
      break;
    }
    listitem = (iONode)ListOp.next( occ );
  }

  if ( depth > 100 || theEnd) {
    TraceOp.trc( name, TRCLEVEL_EXCEPTION, __LINE__, 9999, "ANALYSER: max. recursion depth (100) reached");
    return False;
  }

  int x = 0;
  int y = 0;

  int xoffset = 0;
  int yoffset = 0;

  int turnoutstate_out;

  /* get next item */
  travel = __travel(item, travel, turnoutstate, &turnoutstate_out, &x, &y, "");

  if( travel >= 200 && travel < 300) {
    travel -= twoWayTurnout;
  }
  if( travel >= 300 && travel < 400) {
    travel -= threeWayTurnout;
  }
  if( travel >= 400 && travel < 500) {
    travel -= dcrossing;
  }

  xoffset += x;
  yoffset += y;

  switch(travel) {
    case oriWest:
      xoffset--;
      __createKey( key, item, xoffset, yoffset, 0);
       break;
    case oriNorth:
      yoffset--;
      __createKey( key, item, xoffset, yoffset, 0);
       break;
    case oriEast:
      xoffset++;
      __createKey( key, item, xoffset, yoffset, 0);
       break;
    case oriSouth:
      yoffset++;
      __createKey( key, item, xoffset, yoffset, 0);
       break;
    }

    nextitem = (iONode)MapOp.get( data->objectmap, key);

    if( nextitem != NULL) {

      TraceOp.trc( name, TRCLEVEL_DEBUG, __LINE__, 9999, "next item: %s tos: [%d]",
          NodeOp.getName(nextitem), turnoutstate_out );

      /* occ */
      if( StrOp.equals(NodeOp.getName(item) , "sg" ) ) {

        /*is the signal in our direction ?*/
        Boolean inOurTravel = False;
        if( StrOp.equals( itemori, "west" ) && travel == 0){
          inOurTravel = True;
        } else if( StrOp.equals( itemori, "north" ) && travel == 3){
          inOurTravel = True;
        } else if( StrOp.equals( itemori, "east" ) && travel == 2){
          inOurTravel = True;
        } else if( StrOp.equals( itemori, "south" ) && travel == 1){
          inOurTravel = True;
        }

        TraceOp.trc( name, TRCLEVEL_DEBUG, __LINE__, 9999, "-- > is? [%s] inOurTravel: %d",
                      wItem.getid(item), inOurTravel);

        if( searchingSignal && inOurTravel) {

          TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "signal [%s] -> %d",
              wItem.getid(item), searchingSignal);

          /* add route to occlist */
          ListOp.add( data->bkoccitemlist, (obj)occ);

          searchingSignal = False;

          if( behindABlock) {
            return False;
          }
        }

      } /* if sg */

      int travelp = __travel(nextitem, travel, turnoutstate, &turnoutstate_out, &x, &y, "");
      if( (travelp == itemNotInDirection || travelp == -1) && travelp != dcrossingAhead) {
        TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, " -> stop item not in direction" );
        return False;
      }

      if( StrOp.equals(NodeOp.getName(nextitem) , "bk" ) || StrOp.equals( "seltab", NodeOp.getName(nextitem) )) {

        if( behindABlock) {
          TraceOp.trc( name, TRCLEVEL_DEBUG, __LINE__, 9999, "behind a block");

          return False;
        }

        TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "next is a block: [%s]", wItem.getid(nextitem));
        TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, " -> LIST: block [%s] travel: [%d] depth: [%d] tos: [%d]",
                wItem.getid(nextitem), travel, depth, turnoutstate);

        const char * nextitemori = wItem.getori(nextitem);
        if( nextitemori == NULL) {
          nextitemori = "west";
        }
        const char * state = "-";
        if( StrOp.equals( nextitemori, "west" ) && travel == 2){
          state = "+";
        } else if( StrOp.equals( nextitemori, "north" ) && travel == 1){
          state = "+";
        } else if( StrOp.equals( nextitemori, "east" ) && travel == 0){
          state = "+";
        } else if( StrOp.equals( nextitemori, "south" ) && travel == 3){
          state = "+";
        }

        /* LIST */
        iONode itemA = (iONode)NodeOp.base.clone( nextitem);
        wItem.setstate(itemA, state);
        ListOp.add( route, (obj)itemA );

        /* add route to routelist */
        ListOp.add( data->prelist, (obj)route);

        TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "  ");

        if( !analyserStrict)
          return True;

        /*shall we retun? no, there is something to do -> travel to the signal. if it is there by the user!*/
        behindABlock = True;
        searchingSignal = True;

      } else if ( StrOp.equals(NodeOp.getName(nextitem) , "sw" ) ) {

         /*int travelp = __travel(nextitem, travel, turnoutstate, &turnoutstate_out, &x, &y, ""); */
       TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "next is a switch: [%s] state: [%d] travelp: [%d]",
            wItem.getid(nextitem), turnoutstate_out, travelp);

         turnoutstate = turnoutstate_out;

        if( travelp > 10 && behindABlock){
          TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, " found a turnout [%s] behind the end block: return", wItem.getid(nextitem));
          return False;
        }

        /* item is a turnout -> coming from the points: dive into branches */
        if( travelp >= 200 && travelp < 300) {
          travelp -= twoWayTurnout;
          depth++;

          /* clone the route */
          iOList routecloneA = (iOList)ListOp.base.clone( route);

          TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "going into STRAIGHT branch [%s]", wItem.getid(nextitem));
          __analyseItem(inst, nextitem, route, occ, travel, 0, depth, False, behindABlock);
          TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "going into TURNOUT branch [%s]", wItem.getid(nextitem));
          __analyseItem(inst, nextitem, routecloneA, occ, travel, 1, depth, False, behindABlock);

          return True;
        } else if( travelp >= 300 && travelp < 400) {
          TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "three way turnout");
          TraceOp.trc( name, TRCLEVEL_DEBUG, __LINE__, 9999, "     " );
          travelp -= threeWayTurnout;
          depth++;

          /* clone before recursion! */
          iOList listA = (iOList)ListOp.base.clone( route);
          iOList listB = (iOList)ListOp.base.clone( route);

          __analyseItem(inst, nextitem, route, occ, travel, 0, depth, False, behindABlock);
          __analyseItem(inst, nextitem, listA, occ, travel, 1, depth, False, behindABlock);
          __analyseItem(inst, nextitem, listB, occ, travel, 2, depth, False, behindABlock);
          return True;
        } else if( (travelp >= 400 && travelp < 500) || travelp >= dcrossingAhead) {

          if( travelp >= dcrossingAhead)
            travelp -= dcrossingAhead;
          else
            travelp -= dcrossing;

          depth++;

          TraceOp.trc( name, TRCLEVEL_DEBUG, __LINE__, 9999, "dcrossing travel: %d travelp: %d", travel, travelp );

          /* rows: travel */
          /* cols: itemori */

          const int left[16][2] = {{0,2},{1,2},{0,3},{1,3},
                                   {1,2},{0,3},{1,3},{0,2},
                                   {0,3},{1,3},{0,2},{1,2},
                                   {1,3},{0,2},{1,2},{0,3}};

          const int right[16][2] = {{0,3},{1,2},{0,2},{1,3},
                                    {1,2},{0,2},{1,3},{0,3},
                                    {0,2},{1,3},{0,3},{1,2},
                                    {1,3},{0,3},{1,2},{0,2}};

          int state1 = 0;
          int state2 = 0;

          if( !wSwitch.isdir(nextitem) ) {/* left */
            state1 = left[__getOri(nextitem)*4+travelp][0];
            state2 = left[__getOri(nextitem)*4+travelp][1];
          } else if( wSwitch.isdir(nextitem) ) {/* right */
            state1 = right[__getOri(nextitem)*4+travelp][0];
            state2 = right[__getOri(nextitem)*4+travelp][1];
          }

          /* clone before recursion! */
          iOList listA = (iOList)ListOp.base.clone( route);

          TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "-- > going into %d branch [%s]", state1, wItem.getid(nextitem));
          __analyseItem(inst, nextitem, route, occ, travelp, state1, depth, False, behindABlock);
          TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "-- > going into %d branch [%s]", state2, wItem.getid(nextitem));
          __analyseItem(inst, nextitem, listA, occ, travelp, state2, depth, False, behindABlock);
          return True;
        }

      } /* if bk || sw */

      depth++;
      __analyseItem(inst, nextitem, route, occ, travel, turnoutstate, depth, searchingSignal, behindABlock);

    } else { /*item==NULL*/

      TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "return (nextitem==NULL)");
      return False;
    } /*item?NULL*/


    return True;
}

static void __analyseBlock(iOAnalyse inst, iONode block, const char * inittravel) {
  iOAnalyseData data = Data(inst);
  char key[32] = {'\0'};
  iONode item = NULL;
  int travel;
  int blockleftcounter = 0;

  if( StrOp.equals( wItem.west , inittravel ) ) travel = oriWest;
  if( StrOp.equals( wItem.north, inittravel ) ) travel = oriNorth;
  if( StrOp.equals( wItem.east , inittravel ) ) travel = oriEast;
  if( StrOp.equals( wItem.south, inittravel ) ) travel = oriSouth;

  TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "     " );
  TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "--------------------------------------------------");
  TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "start analyzing block [%s] in [%s][%d] direction ",
      wBlock.getid(block), inittravel, travel);

    int xoffset = 0;
    int yoffset = 0;

    int blocklenghth = 4;
    if( wBlock.issmallsymbol( block))
      blocklenghth = 2;


    if( StrOp.equals( inittravel, "east" ) ) {
      xoffset = blocklenghth-1;
    }

    if( StrOp.equals( inittravel, "south" ) ) {
      yoffset = blocklenghth-1;
    }


    iOList route = ListOp.inst();
    iOList occ = ListOp.inst();

    /* start the recursion */
    int ret = __analyseItem(inst, block, route, occ, travel, 0, 0, False, False);

    TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "end analyzing block [%s] in [%s] direction returned: %d",
        wBlock.getid(block), inittravel, ret);

}

static void __analyseOccList(iOAnalyse inst) {
  iOAnalyseData data = Data(inst);
  iONode model = ModelOp.getModel( data->model);

  iOList occlist = (iOList)ListOp.first( data->bkoccitemlist );
  while(occlist) {

    const char * bk = NULL;
    iONode block = NULL;

    iONode item = (iONode)ListOp.first( occlist );
    while(item) {

      if( StrOp.equals( NodeOp.getName(item), "bk") ) {

        iIBlockBase blockb = ModelOp.getBlock( data->model, wItem.getid(item) );
        block = blockb->base.properties( blockb);

        bk = wItem.getid(item);
      } else {

        iONode node = NULL;
        if( StrOp.equals( NodeOp.getName(item), "tk") ) {
          iOTrack track = ModelOp.getTrack( data->model, wItem.getid(item) );
          node = TrackOp.base.properties(track);
        }
        if( StrOp.equals( NodeOp.getName(item), "fb") ) {
          iOFBack track = ModelOp.getFBack( data->model, wItem.getid(item) );
          node = FBackOp.base.properties(track);
        }
        if( StrOp.equals( NodeOp.getName(item), "sg") ) {
          iOSignal track = ModelOp.getSignal( data->model, wItem.getid(item) );
          node = SignalOp.base.properties(track);
        }
        if( StrOp.equals( NodeOp.getName(item), "sw") ) {
          iOSwitch track = ModelOp.getSwitch( data->model, wItem.getid(item) );
          node = SwitchOp.base.properties(track);

          /*
          const char * prev = wBlock.getturnoutstolock(block);
          if( prev != NULL || !StrOp.equals(prev, "")) {
            iOStrTok tok = StrTokOp.inst( prev, ',' );
            Boolean isInList = False;
            while ( StrTokOp.hasMoreTokens( tok )) {
              const char * token = StrTokOp.nextToken( tok );
              if( StrOp.equals( token, wItem.getid(item))) {
                isInList = True;
              }
            }

            if( !isInList ) {
              char* newval = StrOp.fmt("%s,%s", wBlock.getturnoutstolock(block), wItem.getid(item));

              TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "    locked: [%s] for [%s] ", wItem.getid(item), wItem.getid(block));
              wBlock.setturnoutstolock(block, newval);
              StrOp.free( newval);
            } else {
              wBlock.setturnoutstolock(block, wItem.getid(item));
            }

          }*/

        } /* if sw */

        if( node != NULL) {
          wItem.setblockid(node, bk);
        }
      } /* if bk */

      TraceOp.trc( name, TRCLEVEL_DEBUG, __LINE__, 9999, " OCCITEM: [%s] for: [%s] ", wItem.getid(item), bk);
      item = (iONode)ListOp.next( occlist );
    }

    occlist = (iOList)ListOp.next( data->bkoccitemlist );
  }

}

static void __analyseList(iOAnalyse inst) {
  iOAnalyseData data = Data(inst);
  iONode model = ModelOp.getModel( data->model);
  iONode stlist = wPlan.getstlist(model);



  const char * bka = NULL;
  const char * bkb = NULL;
  const char * bkaside = NULL;
  const char * bkbside = NULL;

  /* SET TO False -> the plan will not be modified!*/
  Boolean doIt = True;

  TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, " ");
  iONode child = NULL;
  iOList delList = ListOp.inst();
  int childcnt = NodeOp.getChildCnt( stlist);
  int i;
  for( i = 0; i <childcnt; i++) {
    child = NodeOp.getChild( stlist, i);

    if( StrOp.startsWith( wItem.getid( child), "autogen-" )) {
      TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999,
          "removed autogen route: [%s]", wItem.getid( child));
      ListOp.add( delList, (obj)child );
    }
  }

  childcnt = ListOp.size(delList);
  for( i = 0; i <childcnt; i++) {
    NodeOp.removeChild( stlist, (iONode)ListOp.get(delList, i) );
  }
  ListOp.base.del(delList);

  TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, " ");
  TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "the analyzer found the routes:");

  int fbcount = 0;

  iOList routelist = (iOList)ListOp.first( data->prelist );
  while(routelist) {

    iONode newRoute = NodeOp.inst( "st", NULL, ELEMENT_NODE );

    TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "route:");
    iONode item = (iONode)ListOp.first( routelist );
    bka = wItem.getid(item);
    bkaside = wItem.getstate(item);


    /* presearch: go to the endblock -> bkb*/
    Boolean reachedEndblock = False;
    Boolean endsonasignal = False;
    Boolean signalreached = False;

    /* Feedback generation */
    Boolean addFeedbacks = False;

    int count = 0;
    while(item) {

      if( (StrOp.equals(NodeOp.getName(item) , "bk" ) || StrOp.equals(NodeOp.getName(item) , "seltab" ) )&& count != 0) {
        bkb = wItem.getid(item);
        bkbside = wItem.getstate(item);
        reachedEndblock = True;
      }

      if( StrOp.equals(NodeOp.getName(item), "sg") && StrOp.equals(wItem.getstate(item), "yes" ) ) {
        endsonasignal = True;
      } else {
        endsonasignal = False;
      }

      count ++;
      item = (iONode)ListOp.next( routelist );
    }

    char* autogenID = StrOp.fmt( "autogen-[%s%s]-[%s%s]", bka, bkaside, bkb, bkbside );
    wRoute.setid( newRoute, autogenID );
    StrOp.free(autogenID);
    wRoute.setbka( newRoute, bka);
    wRoute.setbkb( newRoute, bkb);
    wRoute.setbkaside( newRoute, StrOp.equals( bkaside, "+" )?True:False );
    wRoute.setbkbside( newRoute, StrOp.equals( bkbside, "+" )?True:False );

    Boolean addToList = True;
    for( i = 0; i <NodeOp.getChildCnt( stlist); i++) {
      child = NodeOp.getChild( stlist, i);

      if( StrOp.equals( wRoute.getbka( child), wRoute.getbka( newRoute)) &&
          StrOp.equals( wRoute.getbkb( child), wRoute.getbkb( newRoute)) &&
              wRoute.isbkaside( child) ==  wRoute.isbkaside( newRoute) &&
              wRoute.isbkbside( child) ==  wRoute.isbkbside( newRoute) ) {

        if( !StrOp.equals( wRoute.getid( child), wRoute.getid( newRoute)) ) {
          TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999,
            "found an edited route: [%s] from [%s] to [%s] skip",
            wItem.getid( child), wRoute.getbka( child), wRoute.getbkb( child));
          addToList = False;
          break;
        } else {
          /* second route between two identical blocks found, make unique ID */
          char* extID = StrOp.fmt( "%s-%d", wRoute.getid( newRoute ), i );
          wRoute.setid( newRoute, extID );
          StrOp.free( extID );
        }
      }

      /*
      if( StrOp.equals( wItem.getid( child), wItem.getid( newRoute) ) ){
         TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999,
             "xxx refresh route: [%s]", wItem.getid( child));

         NodeOp.removeChild( stlist, child );
         i = 0;
      }*/
    }

    reachedEndblock = False;
    item = (iONode)ListOp.first( routelist );
    while(item) {

      const char * itemori = wItem.getori(item);
      if( itemori == NULL) {
        itemori = "west";
      }

      TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999,
        " [%s][%s][%s]", NodeOp.getName(item),
        wItem.getid(item), wItem.getstate(item) );

      if( StrOp.equals(wItem.getid(item), bkb) ) {
        TraceOp.trc( name, TRCLEVEL_DEBUG, __LINE__, 9999,
                     "REACHED ENDBLOCK");
        reachedEndblock = True;

        if( !endsonasignal)
          break;
      }

      if( StrOp.equals( NodeOp.getName(item), "sw") ) {
        iONode swcmd = NodeOp.inst( "swcmd", NULL, ELEMENT_NODE );
        wItem.setid( swcmd, wItem.getid(item));
        wSwitch.setcmd( swcmd, wItem.getstate(item));
        NodeOp.addChild( newRoute, swcmd );
      }

      if( StrOp.equals( NodeOp.getName(item), "fb") && endsonasignal && addFeedbacks) {
        TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999,
                             "-------> fb: [%s]", wItem.getid( item));

        if( !signalreached) {
          TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "     is [%s] [%s] event for block [%s]",
              wRoute.isbkaside( newRoute)?"all":"all-reverse", "ENTER",
                  wRoute.getbka( newRoute));

          iONode fbevent = NodeOp.inst( "fbevent", NULL, ELEMENT_NODE );
          wItem.setid( fbevent, wItem.getid(item));
          wFeedbackEvent.setaction( fbevent, "enter");
          wFeedbackEvent.setfrom( fbevent, wRoute.isbkaside( newRoute)?"all":"all-reverse");

          iIBlockBase block = ModelOp.getBlock( data->model, wRoute.getbka( newRoute) );
          iONode blocknode = block->base.properties(block);
          NodeOp.addChild( blocknode, fbevent );
        }


        if( reachedEndblock) {
          TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "     is [%s] [%s] event for block [%s]",
                       wRoute.isbkbside( newRoute)?"all":"all-reverse", "IN",
                           wRoute.getbkb( newRoute));

          iONode fbevent = NodeOp.inst( "fbevent", NULL, ELEMENT_NODE );
          wItem.setid( fbevent, wItem.getid(item));
          wFeedbackEvent.setaction( fbevent, "in");
          wFeedbackEvent.setfrom( fbevent, wRoute.isbkbside( newRoute)?"all":"all-reverse");

          iIBlockBase block = ModelOp.getBlock( data->model, wRoute.getbkb( newRoute) );
          iONode blocknode = block->base.properties(block);
          NodeOp.addChild( blocknode, fbevent );
        }



      }

      if( StrOp.equals( NodeOp.getName(item), "tk") ||
          StrOp.equals( NodeOp.getName(item), "fb") ||
          StrOp.equals( NodeOp.getName(item), "sg") ) {

        iONode tracknode = NULL;

        if( StrOp.equals( NodeOp.getName(item), "tk") ) {
          iOTrack track = ModelOp.getTrack( data->model, wItem.getid(item) );
          tracknode = TrackOp.base.properties(track);
        }

        if( StrOp.equals( NodeOp.getName(item), "fb") ) {
          iOFBack track = ModelOp.getFBack( data->model, wItem.getid(item) );
          tracknode = FBackOp.base.properties(track);
        }

        if( StrOp.equals( NodeOp.getName(item), "sg") ) {
          iOSignal track = ModelOp.getSignal( data->model, wItem.getid(item) );
          tracknode = SignalOp.base.properties(track);

          if( StrOp.equals( wItem.getstate(item), "yes" ) && !reachedEndblock){

            const char * signaltype = wSignal.getsignal( item);
            if( signaltype == NULL)
              signaltype = "main";

            TraceOp.trc( name, TRCLEVEL_DEBUG, __LINE__, 9999,
            " -> the signal [%s][%s] is the [%s] side %s signal for block [%s]", NodeOp.getName(item),
            wItem.getid(item), bkaside, signaltype, bka );

            signalreached = True;

            /* add signals to the block ... */
            iIBlockBase block = ModelOp.getBlock( data->model, bka );
            iONode blocknode = block->base.properties(block);

            if( !StrOp.equals( bkaside, "+") ) {
              if( StrOp.equals( NodeOp.getStr( item, "signal", "main"), "main") ) {
                wBlock.setsignal(blocknode, wItem.getid(item));
                TraceOp.trc( name, TRCLEVEL_DEBUG, __LINE__, 9999,"OK");
              } else if (StrOp.equals( NodeOp.getStr( item, "signal", "distant"), "main") ) {
                wBlock.setwsignal(blocknode, wItem.getid(item));
                TraceOp.trc( name, TRCLEVEL_DEBUG, __LINE__, 9999,"OK");
              }
            } else if( StrOp.equals( bkaside, "+") ) {
              if( StrOp.equals( NodeOp.getStr( item, "signal", "main"), "main") ) {
                wBlock.setsignalR(blocknode, wItem.getid(item));
                TraceOp.trc( name, TRCLEVEL_DEBUG, __LINE__, 9999,"OK");
              } else if (StrOp.equals( NodeOp.getStr( item, "signal", "distant"), "main") ) {
                wBlock.setwsignalR(blocknode, wItem.getid(item));
                TraceOp.trc( name, TRCLEVEL_DEBUG, __LINE__, 9999,"OK");
              }
            }

            TraceOp.trc( name, TRCLEVEL_DEBUG, __LINE__, 9999,
                        "      block [%s] set [%s] %s signal [%s]", bka, bkaside, NodeOp.getStr( item, "signal", "main"),
                        wItem.getid(item) );
          }

        }

        if ( addToList ) {
          char * prevrouteids = StrOp.dup( wItem.getrouteids(tracknode) );
          if( prevrouteids != NULL ) {
            iOStrTok tok = StrTokOp.inst( prevrouteids, ',' );
            /* check if id is all ready in the list */
            Boolean isInList = False;
            while ( StrTokOp.hasMoreTokens( tok )) {
              const char * token = StrTokOp.nextToken( tok );
              if( StrOp.equals( token, wRoute.getid( newRoute))) {
                isInList = True;
              }
            }

            if( !isInList ) {
                if( StrOp.len(prevrouteids)>0 ) {
                  prevrouteids = StrOp.cat( (char*)prevrouteids, ",");
                }
                prevrouteids = StrOp.cat( (char*)prevrouteids, wRoute.getid( newRoute) );
                wItem.setrouteids(tracknode, prevrouteids );
            }

            StrTokOp.base.del(tok);
            } else { /* empty attribute */
            wItem.setrouteids(tracknode, wRoute.getid( newRoute) );
          }
          StrOp.free(prevrouteids);
        }

        if( cleanrun ) {
          /* remove all "autogen-"-routeids */
          char *prevrouteids = StrOp.dup(wItem.getrouteids(tracknode));
          char *userrouteids = StrOp.dup("");

          if( prevrouteids != NULL ) {
            iOStrTok tok = StrTokOp.inst( prevrouteids, ',' );
            while ( StrTokOp.hasMoreTokens( tok )) {
              const char * token = StrTokOp.nextToken( tok );
              /* check if id starts with autogen- */
              if( ( StrOp.len(token) > 0 ) && ( ! StrOp.startsWith( token, "autogen-") ) ) {
                /* not "autogen-" so append to new list*/
                if( StrOp.len(userrouteids)>0 ) {
                  userrouteids = StrOp.cat( userrouteids, ",");
                }
                userrouteids = StrOp.cat( userrouteids, token );
              }
            }
            StrTokOp.base.del(tok);
          }
          wItem.setrouteids(tracknode, userrouteids );
          StrOp.free(userrouteids);
          StrOp.free(prevrouteids);
        }
      } /* tk || fb || sw */

      bkb = wRoute.getbkb( newRoute);
      item = (iONode)ListOp.next( routelist );
    }

    /* merge into stlist */
    if( addToList) {

      if ( !(StrOp.equals( wRoute.getbka(newRoute), wRoute.getbkb(newRoute))) ) {

        if( !cleanrun) {
          NodeOp.addChild( stlist, newRoute );
        }
      } else {
        TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999,"found looproute: [%s] -> check your plan!",
            wRoute.getid(newRoute));
      }
    }

    TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, " ");
    routelist = (iOList)ListOp.next( data->prelist );
  }
}

static char* __readableOri( int ori) {

  if (ori == 0)
    return "west";
  else  if (ori == 1)
    return "north";
  else  if (ori == 2)
    return "east";
  else  if (ori == 3)
    return "south";

  return "";

}


static int * _getConnection( iONode item, int * cons ) {

  const char * itemori = wItem.getori(item);
  const char* type = NodeOp.getName(item);
  const char* subtype = wItem.gettype(item);
  int intori = __getOri(item);

  if( __getType(item) == typeTrackCurve  || (StrOp.equals( type , "fb" ) && wFeedback.iscurve( item))   ) {



  } else if (StrOp.equals( "tk" , type ) && StrOp.equals( subtype , "dir" ) ) {

    cons[0] = 1;
    cons[1] = 0;
    cons[2] = 1;
    cons[3] = 0;

  }



  return cons;
}


static void _analyse(iOAnalyse inst) {
  iOAnalyseData data = Data(inst);
  iONode block = NULL;
  int cx, cy;
  int zlevel = 0;

  MapOp.clear(data->objectmap);
  ListOp.clear(data->prelist);
  ListOp.clear(data->bklist);
  ListOp.clear(data->bkoccitemlist);

  iONode modplan = ModelOp.getModPlan( data->model);
  if( modplan == NULL) {
    TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999,
      "not a modplan" );

    int i;
    for( i = 0; i < 100; i++) { /* ??? */

    iOList list = ModelOp.getLevelItems( data->model, i, &cx, &cy, True);

      if( ListOp.size(list) > 0) {
        TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999,
                 "Trackplan: %d objects at level %d and sizes %d x %d", ListOp.size(list), i, cx, cy );
        __prepare(inst, list, 0,0, False);
      }
    }

  } else {
    iONode mod = wModPlan.getmodule( modplan );
    while( mod != NULL ) {

      iOList list = ModelOp.getLevelItems( data->model, zlevel, &cx, &cy, True);

      TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999,
           "preparing module: %s", wModule.gettitle( mod) );
      __prepare(inst, list, wModule.getx(mod), wModule.gety(mod), True);

      zlevel++;
      mod = wModPlan.nextmodule( modplan, mod );
    };
  }

  TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999,
      "  it contains %d blocks", ListOp.size(data->bklist) );
  


  iONode object = NULL;
  object = (iONode)MapOp.first(data->objectmap);
    while(object) {

      TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999,
            "  ITEM: %s - %s", NodeOp.getName( object), wItem.getid( object));

      /* position */
      int x = wItem.getx( object);
      int y = wItem.gety( object);

      /* size */
      int cx = 1;
      int cy = 1;

      if( StrOp.equals( NodeOp.getName( object), "bk" ) ) {
        cx = 4;
        if( wBlock.issmallsymbol( object) ) {
          cx = 2;
        }
      }

      if( StrOp.equals( NodeOp.getName( object), "sw" ) &&
          !( StrOp.equals( wItem.gettype( object), "left" ) ||
            StrOp.equals( wItem.gettype( object), "right" )) ) {
             cx = 2;
      }

      if( StrOp.equals( wItem.getori( object), "north" ) ||
          StrOp.equals( wItem.getori( object), "south" ) ) {
            int tmp = cx;
            cx = cy;
            cy = tmp;
      }

      char key[32] = {'\0'};

      int tmp = 0;
      int i = 0;

      for (i = 0; i < 4; i++) {

        /* get the neighbor keys */
        switch(i) {
          case 0:
            StrOp.fmtb( key, "%d-%d-%d", x-1, y, wItem.getz( object) ); break; /* west */
          case 1:
            StrOp.fmtb( key, "%d-%d-%d", x, y-1, wItem.getz( object) ); break; /* north */
          case 2:
            StrOp.fmtb( key, "%d-%d-%d", x+cx, y, wItem.getz( object) ); break; /* east */
          case 3:
            StrOp.fmtb( key, "%d-%d-%d", x, y+cy, wItem.getz( object) ); break; /* south */
        }


        iONode item = (iONode)MapOp.get( data->objectmap, key);
        if( item != NULL) {

          /* can we go there? */
          int travel = __travel( item, i, 0, &tmp, &tmp, &tmp, "");

          if( travel == i ) {
            TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999,
              "  - %s==%s %s - %s", __readableOri(i),__readableOri(travel) , NodeOp.getName( item), wItem.getid( item));

          } else {
            TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999,
              "  - %s!=%s %s - %s", __readableOri(i),__readableOri(travel) , NodeOp.getName( item), wItem.getid( item));
          }
        }
      }

      TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999,
                   " ");

      object = (iONode)MapOp.next(data->objectmap);
    }

  block = (iONode)ListOp.first(data->bklist);
  while(block) {

    const char * blockori = wItem.getori(block);

    if(  blockori == NULL) {
      blockori = "west";
    }

    if( StrOp.equals( blockori, "west" ) || StrOp.equals( blockori, "east" ) ) {
      __analyseBlock(inst, block, "west");
      __analyseBlock(inst, block, "east");
    } else if( StrOp.equals( blockori, "north" ) || StrOp.equals( blockori, "south" ) ) {
      __analyseBlock(inst, block, "north");
      __analyseBlock(inst, block, "south");
    }

    block = (iONode)ListOp.next(data->bklist);
  }



  __analyseList(inst);

  if( analyserStrict)
    __analyseOccList(inst);



}

static Boolean _checkPlanHealth(iONode model) {
  char key[64] = {'\0'};
  Boolean healthy = True;
  iOMap xyzMap = MapOp.inst();
  iOMap sensorMap = MapOp.inst();
  iOMap switchMap = MapOp.inst();
  int dbs = NodeOp.getChildCnt(model);
  int i = 0;

  TraceOp.trc( name, TRCLEVEL_WARNING, __LINE__, 9999, "checking plan health..." );

  if( !wCtrl.isuseblockside( wRocRail.getctrl( AppOp.getIni() ) ) ) {
    TraceOp.trc( name, TRCLEVEL_EXCEPTION, __LINE__, 9999, "ERROR: Block side routing is not enabled; The classic method is deprecated." );
    healthy = False;
  }

  /* checking ID's */
  for( i = 0; i < dbs; i++ ) {
    iOMap idMap = MapOp.inst();
    iONode db = NodeOp.getChild( model, i );
    int items = NodeOp.getChildCnt(db);
    int n = 0;
    TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "checking list [%s]...", NodeOp.getName(db) );
    for( n = 0; n < items; n++ ) {
      iONode item = NodeOp.getChild( db, n );

      if( StrOp.equals( wActionCtrl.name(), NodeOp.getName(item) ) ||
          StrOp.equals( wAction.name(), NodeOp.getName(item) ) ||
          StrOp.equals( wLocation.name(), NodeOp.getName(item) ))
      {
        /* Ignore */
        continue;
      }

      /* check the basic addressing */
      if( StrOp.equals( wLoc.name(), NodeOp.getName(item) ) ) {
        if( wLoc.getaddr(item) == 0 && !StrOp.equals(wLoc.getprot(item), wLoc.prot_A) ) {
          TraceOp.trc( name, TRCLEVEL_EXCEPTION, __LINE__, 9999, "ERROR: loco %s has no address set", wItem.getid(item) );
          healthy = False;
        }
      }

      if( StrOp.equals( wFeedback.name(), NodeOp.getName(item) ) ) {
        if( wFeedback.getaddr(item) == 0 ) {
          TraceOp.trc( name, TRCLEVEL_EXCEPTION, __LINE__, 9999, "ERROR: sensor %s has no address set", wItem.getid(item) );
          healthy = False;
        }
        else {
          char* key = FBackOp.createAddrKey( wFeedback.getbus(item), wFeedback.getaddr(item), wFeedback.getiid(item) );
          if( MapOp.haskey(sensorMap, key ) ) {
            iONode sensorItem = (iONode)MapOp.get( sensorMap, key );
            TraceOp.trc( name, TRCLEVEL_EXCEPTION, __LINE__, 9999,
                "ERROR: sensor %s has an already used address %d by %s (%s)",
                wItem.getid(item), wFeedback.getaddr(item), wItem.getid(sensorItem), key );
            healthy = False;
          }
          else {
            MapOp.put( sensorMap, key, (obj)item );
          }
          StrOp.free( key );
        }
      }

      if( StrOp.equals( wSwitch.name(), NodeOp.getName(item) ) ) {
        if( wSwitch.getaddr1(item) == 0 && wSwitch.getport1(item) == 0 ) {
          if( StrOp.equals( wSwitch.gettype(item), wSwitch.crossing ) || StrOp.equals( wSwitch.gettype(item), wSwitch.ccrossing ) ) {
            /* crossing and centered crossing do not need an address */
            TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "INFO: crossing \"%s\" has no address.", wItem.getid(item) );
          }
          else {
            TraceOp.trc( name, TRCLEVEL_EXCEPTION, __LINE__, 9999, "ERROR: switch %s has no address set", wItem.getid(item) );
            healthy = False;
          }
        }
        else {
          char key[32];
          StrOp.fmtb( key, "%d-%d-%d-%s", wSwitch.getaddr1(item), wSwitch.getport1(item), wSwitch.getgate1(item), wItem.getiid(item) );
          if( MapOp.haskey(switchMap, key ) ) {
            iONode switchItem = (iONode)MapOp.get( switchMap, key );
            TraceOp.trc( name, TRCLEVEL_WARNING, __LINE__, 9999,
                "WARNING: switch %s has an already used address %d-%d by %s (%s)",
                wItem.getid(item), wSwitch.getaddr1(item), wSwitch.getport1(item), wItem.getid(switchItem), key );
          }
          else {
            MapOp.put( switchMap, key, (obj)item );
          }

          if( ( StrOp.equals( wSwitch.gettype(item), wSwitch.dcrossing ) || StrOp.equals( wSwitch.gettype(item), wSwitch.threeway ) ) && ( wSwitch.getaddr2(item) > 0 || wSwitch.getport2(item) > 0 ) ) {
            StrOp.fmtb( key, "%d-%d-%d-%s", wSwitch.getaddr2(item), wSwitch.getport2(item), wSwitch.getgate2(item), wItem.getiid(item) );
            if( MapOp.haskey(switchMap, key ) ) {
              iONode switchItem = (iONode)MapOp.get( switchMap, key );
              TraceOp.trc( name, TRCLEVEL_WARNING, __LINE__, 9999,
                  "WARNING: switch %s has an already used second address %d-%d by %s (%s)",
                  wItem.getid(item), wSwitch.getaddr2(item), wSwitch.getport2(item), wItem.getid(switchItem), key );
            }
            else {
              MapOp.put( switchMap, key, (obj)item );
            }
          }
        }
      }

      if( StrOp.equals( wOutput.name(), NodeOp.getName(item) ) ) {
        if( wOutput.getaddr(item) > 0 || wOutput.getport(item) > 0 ) {
          char key[32];
          StrOp.fmtb( key, "%d-%d-%s", wOutput.getaddr(item), wOutput.getport(item), wItem.getiid(item) );
          if( MapOp.haskey(switchMap, key ) ) {
            iONode switchItem = (iONode)MapOp.get( switchMap, key );
            TraceOp.trc( name, TRCLEVEL_WARNING, __LINE__, 9999,
                "WARNING: output %s has an already used address %d-%d by %s (%s)",
                wItem.getid(item), wOutput.getaddr(item), wOutput.getport(item), wItem.getid(switchItem), key );
          }
          else {
            MapOp.put( switchMap, key, (obj)item );
          }
        }
      }

      if( StrOp.equals( wSignal.name(), NodeOp.getName(item) ) ) {
        if( wSignal.getaddr(item) == 0 && wSignal.getport1(item) == 0 ) {
          TraceOp.trc( name, TRCLEVEL_EXCEPTION, __LINE__, 9999, "ERROR: signal %s has no address set", wItem.getid(item) );
          healthy = False;
        }
        else {
          char key[32];
          StrOp.fmtb( key, "%d-%d-%d-%s", wSignal.getaddr(item), wSignal.getport1(item), wSignal.getgate1(item), wItem.getiid(item) );
          if( MapOp.haskey(switchMap, key ) ) {
            iONode switchItem = (iONode)MapOp.get( switchMap, key );
            TraceOp.trc( name, TRCLEVEL_WARNING, __LINE__, 9999,
                "WARNING: signal %s has an already used first address %s by %s (%s)",
                wItem.getid(item), key, wItem.getid(switchItem), key );
          }
          else {
            MapOp.put( switchMap, key, (obj)item );
          }
        }

        if( wSignal.getaddr2(item) > 0 || wSignal.getport2(item) > 0 ) {
          char key[32];
          StrOp.fmtb( key, "%d-%d-%d-%s", wSignal.getaddr2(item), wSignal.getport2(item), wSignal.getgate2(item), wItem.getiid(item) );
          if( MapOp.haskey(switchMap, key ) ) {
            iONode switchItem = (iONode)MapOp.get( switchMap, key );
            TraceOp.trc( name, TRCLEVEL_WARNING, __LINE__, 9999,
                "WARNING: signal %s has an already used second address %s by %s (%s)",
                wItem.getid(item), key, wItem.getid(switchItem), key );
          }
          else {
            MapOp.put( switchMap, key, (obj)item );
          }
        }

        if( wSignal.getaspects(item) >= 3 && ( wSignal.getaddr3(item) > 0 || wSignal.getport3(item) > 0 ) ) {
          char key[32];
          StrOp.fmtb( key, "%d-%d-%d-%s", wSignal.getaddr3(item), wSignal.getport3(item), wSignal.getgate3(item), wItem.getiid(item) );
          if( MapOp.haskey(switchMap, key ) ) {
            iONode switchItem = (iONode)MapOp.get( switchMap, key );
            TraceOp.trc( name, TRCLEVEL_WARNING, __LINE__, 9999,
                "WARNING: signal %s has an already used third address %s by %s (%s)",
                wItem.getid(item), key, wItem.getid(switchItem), key );
          }
          else {
            MapOp.put( switchMap, key, (obj)item );
          }
        }

        if( wSignal.getaspects(item) >= 4 && ( wSignal.getaddr4(item) > 0 || wSignal.getport4(item) > 0 ) ) {
          char key[32];
          StrOp.fmtb( key, "%d-%d-%d-%s", wSignal.getaddr4(item), wSignal.getport4(item), wSignal.getgate4(item), wItem.getiid(item) );
          if( MapOp.haskey(switchMap, key ) ) {
            iONode switchItem = (iONode)MapOp.get( switchMap, key );
            TraceOp.trc( name, TRCLEVEL_WARNING, __LINE__, 9999,
                "WARNING: signal %s has an already used fourth address %s by %s (%s)",
                wItem.getid(item), key, wItem.getid(switchItem), key );
          }
          else {
            MapOp.put( switchMap, key, (obj)item );
          }
        }
      }



      StrOp.fmtb( key, "%d-%d-%d", wItem.getx(item), wItem.gety(item), wItem.getz(item) );

      if( MapOp.haskey(idMap, wItem.getid(item)) ) {
        iONode firstItem = (iONode)MapOp.get(idMap, wItem.getid(item));
        TraceOp.trc( name, TRCLEVEL_EXCEPTION, __LINE__, 9999,
            "ERROR: object [%s] with id [%s] at [%d,%d,%d] already exist at [%d,%d,%d]",
            NodeOp.getName(item), wItem.getid(item),
            wItem.getx(item), wItem.gety(item), wItem.getz(item),
            wItem.getx(firstItem), wItem.gety(firstItem), wItem.getz(firstItem));
        healthy = False;
      }
      else {
        MapOp.put(idMap, wItem.getid(item), (obj)item );
      }

      /* checking overlapping */
      if( wItem.isshow(item) ) {
        if( MapOp.haskey(xyzMap, key) ) {
          iONode firstItem = (iONode)MapOp.get(xyzMap, key);
          TraceOp.trc( name, TRCLEVEL_EXCEPTION, __LINE__, 9999,
              "ERROR: object [%s] with id [%s] at [%d,%d,%d] overlaps object [%s] with id [%s]",
              NodeOp.getName(item), wItem.getid(item),
              wItem.getx(item), wItem.gety(item), wItem.getz(item),
              NodeOp.getName(firstItem), wItem.getid(firstItem));
          healthy = False;
        }

        if( wItem.getx(item) != -1 && wItem.gety(item) != -1 ) {
          if( wItem.getx(item) < -1 || wItem.gety(item) < -1 ) {
            TraceOp.trc( name, TRCLEVEL_EXCEPTION, __LINE__, 9999,
                "ERROR: object [%s] with id [%s] has invalid coordinates [%d,%d,%d]",
                NodeOp.getName(item), wItem.getid(item),
                wItem.getx(item), wItem.gety(item), wItem.getz(item));
            if( wItem.getx(item) < -1 )
              wItem.setx(item, 0);
            if( wItem.gety(item) < -1 )
              wItem.sety(item, 0);
            healthy = False;
          }
          else
            MapOp.put(xyzMap, key, (obj)item );
        }

        if( wItem.getx(item) > 256 || wItem.gety(item) > 256 ) {
          TraceOp.trc( name, TRCLEVEL_EXCEPTION, __LINE__, 9999,
              "ERROR: object [%s] with id [%s] has invalid coordinates [%d,%d,%d]",
              NodeOp.getName(item), wItem.getid(item),
              wItem.getx(item), wItem.gety(item), wItem.getz(item));
          if( wItem.getx(item) > 256 )
            wItem.setx(item, 0);
          if( wItem.gety(item) > 256 )
            wItem.sety(item, 0);
          healthy = False;
        }
      }
      else {
        TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999,
            "INFO: invisible object [%s] with id [%s] and coordinates [%d,%d,%d]",
            NodeOp.getName(item), wItem.getid(item),
            wItem.getx(item), wItem.gety(item), wItem.getz(item));
      }

    }
    MapOp.base.del(idMap);
  }

  /* check for very lonely objects */
  if( MapOp.size(xyzMap) > 0 ) {
    int items = MapOp.size(xyzMap);
    int maxDist = 0;
    iONode lonelyItem = NULL;
    iONode item = (iONode)MapOp.first(xyzMap);
    while( item != NULL ) {
      if( maxDist < wItem.getx(item) + wItem.gety(item) ) {
        /* use x + y coordinates as approach to sqrt( x^2 + y^2 ) */
        maxDist = wItem.getx(item) + wItem.gety(item) ;
        lonelyItem = item;
      }
      item = (iONode)MapOp.next(xyzMap);
    }

    if( lonelyItem != NULL ) {
      TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999,
          "INFO: object [%s] with id [%s] at [%d,%d,%d] is the most far away object in the plan",
          NodeOp.getName(lonelyItem), wItem.getid(lonelyItem),
          wItem.getx(lonelyItem), wItem.gety(lonelyItem), wItem.getz(lonelyItem));
    }
  }

  MapOp.base.del(xyzMap);
  MapOp.base.del(sensorMap);
  MapOp.base.del(switchMap);

  if( healthy ) {
    TraceOp.trc( name, TRCLEVEL_WARNING, __LINE__, 9999, "Plan is healthy" );
  } else {
    TraceOp.trc( name, TRCLEVEL_EXCEPTION, __LINE__, 9999, "Plan is NOT healthty. See ERROR lines above." );
  }

  return healthy;
}



/**  */
static struct OAnalyse* _inst( an_mode Mode ) {
  iOAnalyse __Analyse = allocMem( sizeof( struct OAnalyse ) );
  iOAnalyseData data = allocMem( sizeof( struct OAnalyseData ) );
  MemOp.basecpy( __Analyse, &AnalyseOp, 0, sizeof( struct OAnalyse ), data );

  data->mode = Mode;
  cleanrun = (Mode == AN_CLEAN) ;
  /* Initialize data->xxx members... */
  data->model = AppOp.getModel();
  data->plan  = ModelOp.getModel(data->model);
  data->objectmap = MapOp.inst();
  data->prelist = ListOp.inst();
  data->bklist = ListOp.inst();
  data->bkoccitemlist = ListOp.inst();
  instCnt++;
  return __Analyse;
}

/* ----- DO NOT REMOVE OR EDIT THIS INCLUDE LINE! -----*/
#include "rocrail/impl/analyse.fm"
/* ----- DO NOT REMOVE OR EDIT THIS INCLUDE LINE! -----*/
