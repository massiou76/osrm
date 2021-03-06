/*
    open source routing machine
    Copyright (C) Dennis Luxen, others 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
 */

#ifndef GRAPHLOADER_H
#define GRAPHLOADER_H

#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>

#include <google/dense_hash_map>

#ifdef _GLIBCXX_PARALLEL
#include <parallel/algorithm>
#else
#include <algorithm>
#endif

#include "../DataStructures/ImportEdge.h"
#include "../typedefs.h"

typedef google::dense_hash_map<NodeID, NodeID> ExternalNodeMap;

template<typename EdgeT>
NodeID readOSRMGraphFromStream(istream &in, vector<EdgeT>& edgeList, vector<NodeInfo> * int2ExtNodeMap) {
    NodeID n, source, target, id;
    EdgeID m;
    int dir, xcoord, ycoord;// direction (0 = open, 1 = forward, 2+ = open)
    ExternalNodeMap ext2IntNodeMap;
    ext2IntNodeMap.set_empty_key(UINT_MAX);
    in >> n;
    VERBOSE(cout << "Importing n = " << n << " nodes ..." << flush;)
    for (NodeID i=0; i<n;i++) {
        in >> id >> ycoord >> xcoord;
        int2ExtNodeMap->push_back(NodeInfo(xcoord, ycoord, id));
        ext2IntNodeMap.insert(make_pair(id, i));
    }
    in >> m;
    VERBOSE(cout << " and " << m << " edges ..." << flush;)

    edgeList.reserve(m);
    for (EdgeID i=0; i<m; i++) {
        EdgeWeight weight;
        short type;
        NodeID nameID;
        int length;
        in >> source >> target >> length >> dir >> weight >> type >> nameID;
        assert(length > 0);
        assert(weight > 0);
        assert(0<=dir && dir<=2);

        bool forward = true;
        bool backward = true;
        if (1 == dir) backward = false;
        if (2 == dir) forward = false;

        if(length == 0)
        { cerr << "loaded null length edge" << endl; exit(1); }

        //         translate the external NodeIDs to internal IDs
        ExternalNodeMap::iterator intNodeID = ext2IntNodeMap.find(source);
        if( ext2IntNodeMap.find(source) == ext2IntNodeMap.end())
        {
            cerr << "after " << edgeList.size() << " edges" << endl;
            cerr << "->" << source << "," << target << "," << length << "," << dir << "," << weight << endl;
            cerr << "unresolved source NodeID: " << source << endl; exit(0);
        }
        source = intNodeID->second;
        intNodeID = ext2IntNodeMap.find(target);
        if(ext2IntNodeMap.find(target) == ext2IntNodeMap.end()) { cerr << "unresolved target NodeID : " << target << endl; exit(0); }
        target = intNodeID->second;

        if(source == UINT_MAX || target == UINT_MAX) { cerr << "nonexisting source or target" << endl; exit(0); }

        EdgeT inputEdge(source, target, nameID, weight, forward, backward, type );
        edgeList.push_back(inputEdge);
    }
    ext2IntNodeMap.clear();
    vector<ImportEdge>(edgeList.begin(), edgeList.end()).swap(edgeList); //remove excess candidates.
    cout << "ok" << endl;
    return n;
}
template<typename EdgeT>
NodeID readBinaryOSRMGraphFromStream(istream &in, vector<EdgeT>& edgeList, vector<NodeInfo> * int2ExtNodeMap) {
    NodeID n, source, target, id;
    EdgeID m;
    short dir;
    int xcoord, ycoord;// direction (0 = open, 1 = forward, 2+ = open)
    ExternalNodeMap ext2IntNodeMap;
    ext2IntNodeMap.set_empty_key(UINT_MAX);
    in.read((char*)&n, sizeof(NodeID));
    VERBOSE(cout << "Importing n = " << n << " nodes ..." << flush;)
    for (NodeID i=0; i<n;i++) {
        in.read((char*)&id, sizeof(unsigned));
        in.read((char*)&ycoord, sizeof(int));
        in.read((char*)&xcoord, sizeof(int));
        int2ExtNodeMap->push_back(NodeInfo(xcoord, ycoord, id));
        ext2IntNodeMap.insert(make_pair(id, i));
    }
    in.read((char*)&m, sizeof(unsigned));
    VERBOSE(cout << " and " << m << " edges ..." << flush;)

    edgeList.reserve(m);
    for (EdgeID i=0; i<m; i++) {
        EdgeWeight weight;
        short type;
        NodeID nameID;
        int length;
        in.read((char*)&source, sizeof(unsigned));
        in.read((char*)&target, sizeof(unsigned));
        in.read((char*)&length, sizeof(int));
        in.read((char*)&dir,    sizeof(short));
        in.read((char*)&weight, sizeof(int));
        in.read((char*)&type,   sizeof(short));
        in.read((char*)&nameID ,sizeof(unsigned));

        assert(length > 0);
        assert(weight > 0);
        assert(0<=dir && dir<=2);

        bool forward = true;
        bool backward = true;
        if (1 == dir) { backward = false; }
        if (2 == dir) { forward = false; }

        if(length == 0) { cerr << "loaded null length edge" << endl; exit(1); }

        //         translate the external NodeIDs to internal IDs
        ExternalNodeMap::iterator intNodeID = ext2IntNodeMap.find(source);
        if( ext2IntNodeMap.find(source) == ext2IntNodeMap.end()) {
#ifndef NDEBUG
            cerr << "[warning] unresolved source NodeID: " << source << endl;
#endif
            continue;
        }
        source = intNodeID->second;
        intNodeID = ext2IntNodeMap.find(target);
        if(ext2IntNodeMap.find(target) == ext2IntNodeMap.end()) {
#ifndef NDEBUG
           cerr << "unresolved target NodeID : " << target << endl;
#endif
           continue;
        }
        target = intNodeID->second;

        if(source == UINT_MAX || target == UINT_MAX) { cerr << "nonexisting source or target" << endl; exit(0); }

        EdgeT inputEdge(source, target, nameID, weight, forward, backward, type );
        edgeList.push_back(inputEdge);
    }
    ext2IntNodeMap.clear();
    vector<ImportEdge>(edgeList.begin(), edgeList.end()).swap(edgeList); //remove excess candidates.
    cout << "ok" << endl;
    return n;
}
template<typename EdgeT>
NodeID readDTMPGraphFromStream(istream &in, vector<EdgeT>& edgeList, vector<NodeInfo> * int2ExtNodeMap) {
    NodeID n, source, target, id;
    EdgeID m;
    int dir, xcoord, ycoord;// direction (0 = open, 1 = forward, 2+ = open)
    ExternalNodeMap ext2IntNodeMap;
    ext2IntNodeMap.set_empty_key(UINT_MAX);
    in >> n;
    VERBOSE(cout << "Importing n = " << n << " nodes ..." << flush;)
    for (NodeID i=0; i<n;i++) {
        in >> id >> ycoord >> xcoord;
        int2ExtNodeMap->push_back(NodeInfo(xcoord, ycoord, id));
        ext2IntNodeMap.insert(make_pair(id, i));
    }
    in >> m;
    VERBOSE(cout << " and " << m << " edges ..." << flush;)

    edgeList.reserve(m);
    for (EdgeID i=0; i<m; i++) {
        EdgeWeight weight;
        unsigned speedType(0);
        short type(0);
       // NodeID nameID;
        int length;
        in >> source >> target >> length >> dir >> speedType;

        if(dir == 3)
            dir = 0;

        switch(speedType) {
        case 1:
            weight = 130;
            break;
        case 2:
            weight = 120;
            break;
        case 3:
            weight = 110;
            break;
        case 4:
            weight = 100;
            break;
        case 5:
            weight = 90;
            break;
        case 6:
            weight = 80;
            break;
        case 7:
            weight = 70;
            break;
        case 8:
            weight = 60;
            break;
        case 9:
            weight = 50;
            break;
        case 10:
            weight = 40;
            break;
        case 11:
            weight = 30;
            break;
        case 12:
            weight = 20;
            break;
        case 13:
            weight = length;
            break;
        case 15:
            weight = 10;
            break;
        default:
            weight = 0;
            break;
        }

        weight = length*weight/3.6;
        if(speedType == 13)
            weight = length;
        assert(length > 0);
        assert(weight > 0);
        if(dir <0 || dir > 2)
            std::cerr << "[error] direction bogus: " << dir << std::endl;
        assert(0<=dir && dir<=2);

        bool forward = true;
        bool backward = true;
        if (dir == 1) backward = false;
        if (dir == 2) forward = false;

        if(length == 0)
        { cerr << "loaded null length edge" << endl; exit(1); }

        //         translate the external NodeIDs to internal IDs
        ExternalNodeMap::iterator intNodeID = ext2IntNodeMap.find(source);
        if( ext2IntNodeMap.find(source) == ext2IntNodeMap.end())
        {
            cerr << "after " << edgeList.size() << " edges" << endl;
            cerr << "->" << source << "," << target << "," << length << "," << dir << "," << weight << endl;
            cerr << "unresolved source NodeID: " << source << endl; exit(0);
        }
        source = intNodeID->second;
        intNodeID = ext2IntNodeMap.find(target);
        if(ext2IntNodeMap.find(target) == ext2IntNodeMap.end()) { cerr << "unresolved target NodeID : " << target << endl; exit(0); }
        target = intNodeID->second;

        if(source == UINT_MAX || target == UINT_MAX) { cerr << "nonexisting source or target" << endl; exit(0); }

        EdgeT inputEdge(source, target, 0, weight, forward, backward, type );
        edgeList.push_back(inputEdge);
    }
    ext2IntNodeMap.clear();
    vector<ImportEdge>(edgeList.begin(), edgeList.end()).swap(edgeList); //remove excess candidates.
    cout << "ok" << endl;
    return n;
}

template<typename EdgeT>
NodeID readDDSGGraphFromStream(istream &in, vector<EdgeT>& edgeList, vector<NodeID> & int2ExtNodeMap) {
    ExternalNodeMap nodeMap; nodeMap.set_empty_key(UINT_MAX);
    NodeID n, source, target;
    unsigned numberOfNodes = 0;
    char d;
    EdgeID m;
    int dir;// direction (0 = open, 1 = forward, 2+ = open)
    in >> d;
    in >> n;
    in >> m;
#ifndef DEBUG
    std::cout << "expecting " << n << " nodes and " << m << " edges ..." << flush;
#endif
    edgeList.reserve(m);
    for (EdgeID i=0; i<m; i++) {
        EdgeWeight weight;
        in >> source >> target >> weight >> dir;

//        if(dir == 3)
//            dir = 0;

        assert(weight > 0);
        if(dir <0 || dir > 3)
            std::cerr << "[error] direction bogus: " << dir << std::endl;
        assert(0<=dir && dir<=3);

        bool forward = true;
        bool backward = true;
        if (dir == 1) backward = false;
        if (dir == 2) forward = false;
        if (dir == 3) {backward = true; forward = true;}

        if(weight == 0)
        { cerr << "loaded null length edge" << endl; exit(1); }

        if( nodeMap.find(source) == nodeMap.end()) {
            nodeMap.insert(std::make_pair(source, numberOfNodes ));
            int2ExtNodeMap.push_back(source);
            numberOfNodes++;
        }
        if( nodeMap.find(target) == nodeMap.end()) {
            nodeMap.insert(std::make_pair(target, numberOfNodes));
            int2ExtNodeMap.push_back(target);
            numberOfNodes++;
        }
        EdgeT inputEdge(source, target, 0, weight, forward, backward, 1 );
        edgeList.push_back(inputEdge);
   }
    vector<EdgeT>(edgeList.begin(), edgeList.end()).swap(edgeList); //remove excess candidates.

//    cout << "ok" << endl;
//    std::cout << "imported " << numberOfNodes << " nodes and " << edgeList.size() << " edges" << std::endl;
    nodeMap.clear();
    return numberOfNodes;
}

template<typename EdgeT>
unsigned readHSGRFromStream(istream &in, vector<EdgeT> & edgeList) {
    unsigned numberOfNodes = 0;
    ExternalNodeMap nodeMap; nodeMap.set_empty_key(UINT_MAX);
    while(!in.eof()) {
        EdgeT g;
        EdgeData e;

        int distance;
        bool shortcut;
        bool forward;
        bool backward;
        short type;
        NodeID middle;
        NodeID source;
        NodeID target;

        in.read((char *)&(distance), sizeof(int));
        assert(distance > 0);
        in.read((char *)&(shortcut), sizeof(bool));
        in.read((char *)&(forward), sizeof(bool));
        in.read((char *)&(backward), sizeof(bool));
        in.read((char *)&(middle), sizeof(NodeID));
        in.read((char *)&(type), sizeof(short));
        in.read((char *)&(source), sizeof(NodeID));
        in.read((char *)&(target), sizeof(NodeID));
        e.backward = backward; e.distance = distance; e.forward = forward; e.middleName.middle = middle; e.shortcut = shortcut; e.type = type;
        g.data = e;
        g.source = source; g.target = target;

        if(source > numberOfNodes)
            numberOfNodes = source;
        if(target > numberOfNodes)
            numberOfNodes = target;
        if(middle > numberOfNodes)
            numberOfNodes = middle;

        edgeList.push_back(g);
    }
    return numberOfNodes+1;
}

template<typename EdgeT>
unsigned readHSGRFromStreamWithOutEdgeData(const char * hsgrName, vector<EdgeT> * edgeList) {
    std::ifstream hsgrInStream(hsgrName, std::ios::binary);
    unsigned numberOfNodes = 0;
    ExternalNodeMap nodeMap; nodeMap.set_empty_key(UINT_MAX);
    while(!hsgrInStream.eof()) {
        EdgeT g;
//        EdgeData e;

        int distance;
        bool shortcut;
        bool forward;
        bool backward;
        short type;
        NodeID middle;
        NodeID source;
        NodeID target;

        hsgrInStream.read((char *)&(distance), sizeof(int));
        assert(distance > 0);
        hsgrInStream.read((char *)&(shortcut), sizeof(bool));
        hsgrInStream.read((char *)&(forward), sizeof(bool));
        hsgrInStream.read((char *)&(backward), sizeof(bool));
        hsgrInStream.read((char *)&(middle), sizeof(NodeID));
        hsgrInStream.read((char *)&(type), sizeof(short));
        hsgrInStream.read((char *)&(source), sizeof(NodeID));
        hsgrInStream.read((char *)&(target), sizeof(NodeID));
        g.data.backward = backward; g.data.distance = distance; g.data.forward = forward; g.data.shortcut = shortcut;
        g.source = source; g.target = target;

        if( nodeMap.find(source) == nodeMap.end()) {
            nodeMap[numberOfNodes] = source;
            numberOfNodes++;
        }
        if( nodeMap.find(target) == nodeMap.end()) {
            nodeMap[numberOfNodes] = target;
            numberOfNodes++;
        }

        edgeList->push_back(g);
    }
    hsgrInStream.close();
    return numberOfNodes;
}
#endif // GRAPHLOADER_H
