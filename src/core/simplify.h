#pragma once
#include "quadric.h"
struct CollapseStats { int done=0,link=0,flip=0,degenerate=0; };
Mesh simplify_mesh(const Mesh&,int,CollapseStats&, bool unsafe = false);
