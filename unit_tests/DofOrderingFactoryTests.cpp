//
//  DofOrderingFactoryTests
//  Camellia
//
//  Created by Nate Roberts on 4/8/15
//
//

#include "Teuchos_UnitTestHarness.hpp"

#include "TypeDefs.h"

#include "CellTopology.h"
#include "DofOrderingFactory.h"
#include "PoissonFormulation.h"
#include "TensorBasis.h"

using namespace Camellia;
using namespace std;

namespace {
  TEUCHOS_UNIT_TEST( DofOrderingFactory, SpaceTimeTestsUseHGRADInTime )
  {
    int spaceDim = 3;
    bool useConformingTraces = false;
    PoissonFormulation form(spaceDim, useConformingTraces);
    BFPtr bf = form.bf();
    
    DofOrderingFactory factory(bf);
    
    vector<int> polyOrder(2); // space, time
    polyOrder[0] = 2;
    polyOrder[1] = 3;
    
    CellTopoPtr hexTopo = CellTopology::hexahedron();
    int tensorialDegree = 1;
    CellTopoPtr spaceTimeTopo = CellTopology::cellTopology(hexTopo, tensorialDegree);
    
    VarPtr q = form.q();
    VarPtr tau = form.tau();
    
    DofOrderingPtr testOrdering = factory.testOrdering(polyOrder, spaceTimeTopo);
    
    BasisPtr qBasis = testOrdering->getBasis(q->ID());
    BasisPtr tauBasis = testOrdering->getBasis(tau->ID());
    
    TensorBasis<double>* qTensorBasis = dynamic_cast< TensorBasis<double>* >(qBasis.get());
    TensorBasis<double>* tauTensorBasis = dynamic_cast< TensorBasis<double>* >(tauBasis.get());
    
    TEUCHOS_TEST_FOR_EXCEPTION(qTensorBasis == NULL, std::invalid_argument, "qBasis is not a TensorBasis instance");
    TEUCHOS_TEST_FOR_EXCEPTION(tauTensorBasis == NULL, std::invalid_argument, "tauBasis is not a TensorBasis instance");
    
    BasisPtr qSpatialBasis = qTensorBasis->getSpatialBasis();
    TEST_EQUALITY(qSpatialBasis->functionSpace(), efsForSpace(q->space()));
    
    BasisPtr tauSpatialBasis = tauTensorBasis->getSpatialBasis();
    TEST_EQUALITY(tauSpatialBasis->functionSpace(), efsForSpace(tau->space()));
    
    BasisPtr qTemporalBasis = qTensorBasis->getTemporalBasis();
    TEST_EQUALITY(qTemporalBasis->functionSpace(), efsForSpace(HGRAD));
    
    BasisPtr tauTemporalBasis = tauTensorBasis->getTemporalBasis();
    TEST_EQUALITY(tauTemporalBasis->functionSpace(), efsForSpace(HGRAD));
  }
} // namespace