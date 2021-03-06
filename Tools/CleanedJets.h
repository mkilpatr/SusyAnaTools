// CleanedJets.h
// Caleb J. Smith
// October 22, 2018

// Input vector of TLorentzVector: objects (photons, leptons, etc)
// Input vector of TLorentzVector: jets (before they are clean)
// Return vector of TLorentzVector: cleaned jets (dR cleaning)


#ifndef CLEANEDJETS_H
#define CLEANEDJETS_H

#include "SusyAnaTools/Tools/NTupleReader.h"
#include "SusyAnaTools/Tools/customize.h"
#include "Math/VectorUtil.h"
#include "SusyAnaTools/Tools/SusyUtility.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <iterator>

class CleanedJets {

private:
    // AK4 jet variables
    std::vector<std::string> AK4JetVariables_;
    // AK8 jet variables
    std::vector<std::string> AK8JetVariables_;
    
    NTupleReader* tr_;
    void setReader(NTupleReader& tr) { tr_ = &tr; }

    void generateCleanedJets() 
    {
        //                  jetCollectionName, objectNameString,    jetCollectionVariables, prefix,     suffix,               doDRCleaning, doJetCuts
        cleanJetCollection("jetsLVec",      "gammaLVecPassLooseID", AK4JetVariables_, "",               "_drPhotonCleaned",           true, false);
        cleanJetCollection("jetsLVec",      "cutMuVec;cutElecVec",  AK4JetVariables_, "",               "_drLeptonCleaned",           true, false);
        cleanJetCollection("jetsLVec",      "",                     AK4JetVariables_, "",               "_pt20eta24",                 false, true);
        cleanJetCollection("jetsLVec",      "",                     AK4JetVariables_, "prodJetsNoLep_", "_pt20eta24",                 false, true);
        cleanJetCollection("jetsLVec",      "cutMuVec;cutElecVec",  AK4JetVariables_, "",               "_drLeptonCleaned_pt20eta24", true, true);
        cleanJetCollection("puppiJetsLVec", "gammaLVecPassLooseID", AK8JetVariables_, "",               "_drPhotonCleaned",           true, false);
        cleanJetCollection("puppiJetsLVec", "cutMuVec;cutElecVec",  AK8JetVariables_, "",               "_drLeptonCleaned",           true, false);
        cleanJetCollection("puppiJetsLVec", "",                     AK8JetVariables_, "",               "_pt20eta24",                 false, true);
        cleanJetCollection("puppiJetsLVec", "",                     AK8JetVariables_, "prodJetsNoLep_", "_pt20eta24",                 false, true);
        cleanJetCollection("puppiJetsLVec", "cutMuVec;cutElecVec",  AK8JetVariables_, "",               "_drLeptonCleaned_pt20eta24", true, true);
    }

    template <class type> void cleanVector(std::string vectorName, std::vector<bool> keepJet, const std::string& suffix)
    {
        const auto& vec = tr_->getVec<type>(vectorName); 
        std::vector<type>* cleanedVec = new std::vector<type>();
        if (keepJet.size() != vec.size())
        {
            std::cout << "ERROR in cleanVector(): vector sizes do not match for " << vectorName << std::endl;
            std::cout << "In cleanVector(): keepJet.size() = " << keepJet.size() << " and vec.size() = " << vec.size() << std::endl;
        }
        for (int i = 0; i < vec.size(); ++i)
        {
            if (keepJet[i]) cleanedVec->push_back(vec[i]);
        }
        tr_->registerDerivedVec(vectorName + suffix, cleanedVec);
    }

    // clean all variables in jet  collection
    void cleanJetCollection(const std::string& jetCollectionName, const std::string& objectNameString, const std::vector<std::string>& jetCollectionVariables, const std::string& prefix, const std::string& suffix, bool doDRCleaning, bool doJetCuts)
    {
        std::vector<std::string> objectNameVec = SusyUtility::getVecFromString(objectNameString, ';'); 

        const auto& jetsLVec  = tr_->getVec<TLorentzVector>(prefix + jetCollectionName);  // jet lorentz vector
        std::vector<float>* dRvec = new std::vector<float>();
        const float dRMax = 0.20; // dR between photon and jet

        // vector determining which jets to keep 
        std::vector<bool> keepJet(jetsLVec.size(), true);
        // loop over objects: remove jets using DR matching
        if (doDRCleaning)
        {
            for (const auto& objectName : objectNameVec)
            {
                const auto& objectLVec = tr_->getVec<TLorentzVector>(objectName);

                for (int i = 0; i < objectLVec.size(); ++i)
                {
                    int match = AnaFunctions::jetObjectdRMatch(objectLVec[i], jetsLVec, dRMax, dRvec);
                    if (match >= 0) keepJet[match] = false;
                }
            }
        }

        // apply jet p_t and eta cuts
        if(doJetCuts)
        {
            for (int i = 0; i < jetsLVec.size(); i++)
            {
                if ( ! AnaFunctions::jetPassCuts(jetsLVec[i], AnaConsts::pt20Eta24Arr) ) keepJet[i] = false; 
            }
        }
        
        // clean all variables in jet collection
        for (const auto& jetVariable : jetCollectionVariables)
        {
            // TLorentzVector
            if (jetVariable.compare("jetsLVec") == 0 || jetVariable.compare("puppiJetsLVec") == 0)
            {
                cleanVector<TLorentzVector>(prefix + jetVariable, keepJet, suffix);
            }
            // std::vector<TLorentzVector> 
            else if (jetVariable.compare("puppiAK8SubjetLVec") == 0)
            {
                cleanVector<std::vector<TLorentzVector>>(prefix + jetVariable, keepJet, suffix);
            }
            // int
            else if (jetVariable.compare("qgMult") == 0)
            {
                cleanVector<int>(prefix + jetVariable, keepJet, suffix);
            }
            // float
            else
            {
                cleanVector<float>(prefix + jetVariable, keepJet, suffix);
            }
        }
        // dR between jets and photon
        tr_->registerDerivedVec("dR_" + prefix + jetCollectionName + suffix, dRvec);
    }

public:

    CleanedJets()
    {
        // AK4 jet variables
        AK4JetVariables_ = {
                             "jetsLVec",              
                             "recoJetsCSVv2",
                             "qgLikelihood",
                             "qgPtD",                               
                             "qgAxis1",                             
                             "qgAxis2",                             
                             "qgMult",                              
                             "recoJetschargedHadronEnergyFraction", 
                             "recoJetschargedEmEnergyFraction",     
                             "recoJetsneutralEmEnergyFraction",     
                             "recoJetsmuonEnergyFraction",          
                             "recoJetsHFHadronEnergyFraction",     
                             "recoJetsHFEMEnergyFraction",          
                             "recoJetsneutralEnergyFraction",       
                             "PhotonEnergyFraction",                
                             "ElectronEnergyFraction",             
                             "ChargedHadronMultiplicity",          
                             "NeutralHadronMultiplicity",          
                             "PhotonMultiplicity",                
                             "ElectronMultiplicity",               
                             "MuonMultiplicity",                    
                             "DeepCSVb",                            
                             "DeepCSVc",                            
                             "DeepCSVl",                            
                             "DeepCSVbb",                           
                             "DeepCSVcc"
                           };
        
        // AK8 jet variables
        AK8JetVariables_ = {
                             "puppiJetsLVec",              
                             "puppiAK8SubjetLVec",              
                             "puppisoftDropMass",              
                             "puppitau1",
                             "puppitau2",
                             "puppitau3",
                             "deepAK8btop",              
                             "deepAK8bW"
                           };
    }

    ~CleanedJets(){}

    void operator()(NTupleReader& tr)
    {
        // first set NTupleReader
        setReader(tr);
        generateCleanedJets();
    }
};

#endif
