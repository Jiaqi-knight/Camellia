//
//  GDAMinimumRule.h
//  Camellia-debug
//
//  Created by Nate Roberts on 1/21/14.
//
//

#ifndef __Camellia_debug__GDAMinimumRule__
#define __Camellia_debug__GDAMinimumRule__

#include <iostream>

#include "GlobalDofAssignment.h"

#include "LocalDofMapper.h"

#include "BasisReconciliation.h"

struct ConstrainingSubcellInfo {
  GlobalIndexType cellID;
  unsigned sideOrdinal;
  unsigned subcellOrdinalInSide; // when subcell is a side, this is 0
  unsigned dimension; // subcells can be constrained by subcells of higher dimension (i.e. this is not redundant!)
};

struct OwnershipInfo {
  GlobalIndexType cellID;
  GlobalIndexType owningSubcellEntityIndex;
  unsigned dimension;
};

struct CellConstraints {
  vector< vector< ConstrainingSubcellInfo > > subcellConstraints; // outer: subcell dim, inner: subcell ordinal in cell
  vector< vector< OwnershipInfo > > owningCellIDForSubcell; // outer vector indexed by subcell dimension; inner vector indexed by subcell ordinal in cell.  Pairs are (CellID, subcellIndex in MeshTopology)
};

class GDAMinimumRule : public GlobalDofAssignment {
  BasisReconciliation _br;
  map<GlobalIndexType, IndexType> _cellDofOffsets; // (cellID -> first partition-local dof index for that cell)  within the partition, offsets for the owned dofs in cell
  map<GlobalIndexType, GlobalIndexType> _globalCellDofOffsets; // (cellID -> first global dof index for that cell)
  GlobalIndexType _partitionDofOffset; // add to partition-local dof indices to get a global dof index
  GlobalIndexType _partitionDofCount; // how many dofs belong to the local partition
  FieldContainer<IndexType> _partitionDofCounts; // how many dofs belong to all MPI ranks.
  GlobalIndexType _globalDofCount;
  
  map< GlobalIndexType, CellConstraints > _constraintsCache;
  
  map< GlobalIndexType, LocalDofMapperPtr > _dofMapperCache;

  typedef map<int, vector<GlobalIndexType> > VarIDToDofIndices; // key: varID
  typedef map<unsigned, VarIDToDofIndices> SubCellOrdinalToMap; // key: subcell ordinal
  typedef vector< SubCellOrdinalToMap > SubCellDofIndexInfo; // index to vector: subcell dimension
    
  vector<unsigned> allBasisDofOrdinalsVector(int basisCardinality);
  
  void filterSubBasisConstraintData(set<unsigned> &basisDofOrdinals,vector<GlobalIndexType> &globalDofOrdinals,
                                    FieldContainer<double> &constraintMatrixSideInterior, FieldContainer<bool> &processedDofs,
                                    DofOrderingPtr trialOrdering, VarPtr var, int sideOrdinal = 0);
  
  CellConstraints getCellConstraints(GlobalIndexType cellID);
//  CellConstraints getCellConstraintsNew(GlobalIndexType cellID);

  typedef vector< SubBasisDofMapperPtr > BasisMap;
  BasisMap getBasisMap(GlobalIndexType cellID, SubCellDofIndexInfo& dofOwnershipInfo, VarPtr var);
  BasisMap getBasisMap(GlobalIndexType cellID, SubCellDofIndexInfo& dofOwnershipInfo, VarPtr var, int sideOrdinal);
  
  LocalDofMapperPtr getDofMapper(GlobalIndexType cellID, CellConstraints &constraints, int varIDToMap = -1, int sideOrdinalToMap = -1);
  
  SubCellDofIndexInfo getOwnedGlobalDofIndices(GlobalIndexType cellID, CellConstraints &cellConstraints);
  
  int H1Order(GlobalIndexType cellID, unsigned sideOrdinal); // this is meant to track the cell's interior idea of what the H^1 order is along that side.  We're isotropic for now, but eventually we might want to allow anisotropy in p...
  
  RefinementBranch volumeRefinementsForSideEntity(IndexType sideEntityIndex);
public:
  GDAMinimumRule(MeshTopologyPtr meshTopology, VarFactory varFactory, DofOrderingFactoryPtr dofOrderingFactory, MeshPartitionPolicyPtr partitionPolicy,
                 unsigned initialH1OrderTrial, unsigned testOrderEnhancement);
  
  void didHRefine(const set<GlobalIndexType> &parentCellIDs);
  void didPRefine(const set<GlobalIndexType> &cellIDs, int deltaP);
  void didHUnrefine(const set<GlobalIndexType> &parentCellIDs);
  
  void didChangePartitionPolicy();
  
  ElementTypePtr elementType(GlobalIndexType cellID);
  
  int getH1Order(GlobalIndexType cellID);
  GlobalIndexType globalDofCount();
  set<GlobalIndexType> globalDofIndicesForPartition(PartitionIndexType partitionNumber);
  void interpretLocalData(GlobalIndexType cellID, const FieldContainer<double> &localDofs,
                          FieldContainer<double> &globalDofs, FieldContainer<GlobalIndexType> &globalDofIndices, bool accumulate=true);
  void interpretLocalBasisData(GlobalIndexType cellID, int varID, int sideOrdinal, const FieldContainer<double> &basisDofs,
                               FieldContainer<double> &globalDofs, FieldContainer<GlobalIndexType> &globalDofIndices);
  void interpretGlobalData(GlobalIndexType cellID, FieldContainer<double> &localDofs, const Epetra_Vector &globalDofs, bool accumulate=true);
  IndexType localDofCount(); // local to the MPI node

  PartitionIndexType partitionForGlobalDofIndex( GlobalIndexType globalDofIndex );
  void printConstraintInfo(GlobalIndexType cellID);
  void rebuildLookups();
};

#endif /* defined(__Camellia_debug__GDAMinimumRule__) */
