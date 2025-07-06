//=======================================InstantiateUDF======================================//
// Instantiates the UDF from product name and rev and UDF name
HRESULT ProcessToolOperationsBaseClass::InstantiateUDF(const CATBaseUnknown_var & spPrtCont, CATListValCATBaseUnknown_var listOfInputsToInstantiateUDF, PackageType packageType)
{
if (this->IsUDFInstantiated())
{
MRFGenericUtils::ShowErrorMessage("UDF is already instantiated");
return E_FAIL;
}

// listOfInputsToInstantiateUDF -> this variable is not used as of now
HRESULT rc = E_FAIL;
CATBaseUnknown_var spBeltUDFInstance;
int udfIndex = this->GetIndexOfUDFFromCombo();
CATListPV listOfUDFInfo = this->GetListOfUDFInfo();
CATIPLMProducts *pPLMProdutcs_RefUDF = NULL;
CATIUdfInstantiate* pUDFInst_Ref = NULL;

//CATBaseUnknown_var spUDFInstance;

if (listOfUDFInfo.Size() >= udfIndex && udfIndex >= 0)
{
UDFInfo *pudfInfo = (UDFInfo*)listOfUDFInfo[udfIndex];
if (pudfInfo)
{

CATIPLMNavRepReference* opRepRefOfProduct = NULL;
CATUnicodeString udfNameToInst = pudfInfo->udfName, udfProdRefName = pudfInfo->udfProductName, udfProductRev = pudfInfo->udfProductRev;

CATIAdpPLMIdentificator * osSearchedProductId = NULL, *osSearchedProductIDExist = NULL;

int nSearchBeginning = 0;
CATUnicodeString::CATSearchMode searchMode = CATUnicodeString::CATSearchMode::CATSearchModeForward;
int nSearchIndex = udfProductRev.SearchSubString(".", nSearchBeginning, searchMode);

if (nSearchIndex != -1)
udfProductRev = udfProductRev.SubString(nSearchBeginning, nSearchIndex);

rc = MRFGenericUtils::SearchObjectInDatabase(udfProdRefName, udfProductRev, osSearchedProductId);

if (FAILED(rc))
{
MRFGenericUtils::ShowErrorMessage("Reference Product or Part does not exist\n");
}

CATOmbLifeCycleRootsBag lifeCycleRootSessionBag;
CATAdpOpener adpOpener(lifeCycleRootSessionBag);

if (SUCCEEDED(rc))
rc = adpOpener.CompleteAndOpen(osSearchedProductId, IID_CATIPLMProducts, (void**)&pPLMProdutcs_RefUDF);

if (!pPLMProdutcs_RefUDF)
return rc;

if (osSearchedProductId)
osSearchedProductId->Release(); osSearchedProductId = NULL;

CATIPLMNavRepReference* spNavRepRef = NULL;

CATIPLMNavReference *pChildRef = NULL;
rc = pPLMProdutcs_RefUDF->QueryInterface(IID_CATIPLMNavReference, (void**)&pChildRef);

if (pChildRef)
{
MRFProdStrServices::GetRepRefOfInst(pChildRef, spNavRepRef);

pChildRef->Release();
pChildRef = NULL;
}

if (!!spNavRepRef)
{
if (MRFGenericUtils::LoadIntoEditMode(spNavRepRef))
{
CATIMmiPrtContainer * pIPrtContOnReferenceUDFPrd = NULL;
rc = spNavRepRef->RetrieveApplicativeContainer("CATPrtCont", IID_CATIMmiPrtContainer, (void **)&pIPrtContOnReferenceUDFPrd);

spNavRepRef->Release(); spNavRepRef = NULL;

// 4-1 Retrieves the list of user defined feature references from the container
// --------------------------------------------------------------------
CATLISTV(CATIUdfInstantiate_var) iListOfUserFeatureReferences;
if (pIPrtContOnReferenceUDFPrd)
{
CATTemplatesAccessServices::GetUserFeatureList(pIPrtContOnReferenceUDFPrd, iListOfUserFeatureReferences); //Get user feature from Reference Prd using container retrieved from navRepRef

// No more need of this CATIMmiPrtContainer interface pointer
pIPrtContOnReferenceUDFPrd->Release();
pIPrtContOnReferenceUDFPrd = NULL;
}
if (iListOfUserFeatureReferences.Size() <= 0)
return rc = E_FAIL;


// -------------------------------------------------------
// 4-2 Retrieves the User Feature reference to instantiate
// -------------------------------------------------------

bool isUdfFound = false;

if (iListOfUserFeatureReferences.Size() > 0)
{
for (int i = 1; i <= iListOfUserFeatureReferences.Size() && isUdfFound == false; i++)
{
pUDFInst_Ref = iListOfUserFeatureReferences[i];
if (!!pUDFInst_Ref)
{
CATIAlias_var spAlias = (CATBaseUnknown*)pUDFInst_Ref;

if (spAlias->GetAlias().Compare(udfNameToInst))
{
isUdfFound = true;
}
}
else
continue;
}


}

if (isUdfFound == false)
{
MRFGenericUtils::ShowErrorMessage("No UDF Found with the given name\n");
return rc = E_FAIL;
}

}
}


if (SUCCEEDED(rc) && !!pPLMProdutcs_RefUDF && !!pUDFInst_Ref)
{

//-------------------------------------------------------------------
// 3-bis Lock the Representation Reference containing the reference to instantiate
//-------------------------------------------------------------------
CATIPLMComponent_var piPLMCompOnFeatRef;
rc = pPLMProdutcs_RefUDF->QueryInterface(IID_CATIPLMComponent, (void**)&piPLMCompOnFeatRef);
CATOmbLifeCycleRootsBag Bag;

if (rc == S_OK)
Bag.InsertRoot(piPLMCompOnFeatRef);
else
return rc;

CATIMmiPrtContainer_var spPrtContainer = spPrtCont;

CATIMmiMechanicalFeature_var spMechFeatOnPart;


if (spPrtContainer != NULL_var)
rc = spPrtContainer->GetMechanicalPart(spMechFeatOnPart); //Get the destination mechaicalPart i.e Current or working prd

if (FAILED(rc) || (NULL_var == spMechFeatOnPart)) return rc;

if (NULL_var == _spMechFeatOnPart)
_spMechFeatOnPart = spMechFeatOnPart;

CATIPartRequest_var spPartRequestOnCAAUdfModelPart = spMechFeatOnPart;


CATBaseUnknown_var spCAAUdfModelPart = spPartRequestOnCAAUdfModelPart;
CATBaseUnknown* pCAAUdfModelPart = (CATBaseUnknown*)spCAAUdfModelPart;

CATListOfCATUnicodeString * pListOfInputRole = NULL;
CATListValCATBaseUnknown_var * pListOfInput = NULL;

rc = pUDFInst_Ref->GetOldInputs(pListOfInput, pListOfInputRole);
if (FAILED(rc) || (NULL == pListOfInput) || (NULL == pListOfInputRole)) return rc;

int inputListSize = 0;
inputListSize = pListOfInput->Size();

if (inputListSize <= 0)
MRFGenericUtils::ShowErrorMessage("Problem in retrieving the inputs of UDF");


CATPathElement PathFirstInstantiate1(pCAAUdfModelPart);

// In this case the 2 last values are set to null
CATPathElement * FirstUIactiveObject1 = NULL;
CATBaseUnknown_var FirstDest1 = NULL_var;

rc = pUDFInst_Ref->SetDestinationPath(&PathFirstInstantiate1,
FirstUIactiveObject1,
FirstDest1);
if (FAILED(rc)) return rc;

if (NULL != FirstUIactiveObject1)
{
FirstUIactiveObject1->Release();
FirstUIactiveObject1 = NULL;
}

//---------------------Set the UDF Input here----------------------//

//CATListValCATBaseUnknown_var listOfUDFInputsToSet;

if (listOfInputsToInstantiateUDF.Size() == 1)
{
CATUnicodeString nameOfFeat;
CATBaseUnknown_var spXYPlane = listOfInputsToInstantiateUDF[1];
if (!!spXYPlane)
nameOfFeat = MRFGenericUtils::GetAliasName(spXYPlane);

if (!pListOfInputRole->Locate(nameOfFeat)) // if XYPlane not found as one of the input in the UDF needed to be instantiated then remove the XY plane which is given as one input
listOfInputsToInstantiateUDF.RemovePosition(1);
}

//The belwo function retrieves the list of input to map to the UDF instance from the previoius package
this->GetUDFInputsFromPrevPackage(pListOfInput, _spPrevPackUDFInputParmsListToCheck, _spPrevPackUDFOutPutListToCheck, listOfInputsToInstantiateUDF);

delete pListOfInput; pListOfInput = NULL;

if (listOfInputsToInstantiateUDF.Size() >0 && listOfInputsToInstantiateUDF.Size() == inputListSize)
{
pudfInfo->spListToInstantiateUDF = listOfInputsToInstantiateUDF; //Store UDF Info (inputs required to instantiate the UDF)

for (int i = 1; i <= listOfInputsToInstantiateUDF.Size(); i++)
{
CATBaseUnknown_var spBeadInput = listOfInputsToInstantiateUDF[i];
if (!spBeadInput)
return E_FAIL;

CATPathElement * pPathFirstInput1 = new CATPathElement(spBeadInput); //Set Created Axis sytsem XY palne
if (!pPathFirstInput1)
return E_FAIL;

rc = pUDFInst_Ref->SetNewInput(i, pPathFirstInput1);
if (FAILED(rc)) return rc;

pPathFirstInput1->Release();
pPathFirstInput1 = NULL;
}
}
else
{
MRFGenericUtils::ShowErrorMessage("Input List is NULL, pls check the inputs");
return E_FAIL;
}


rc = pUDFInst_Ref->Instantiate(NULL_var);
if (FAILED(rc)) return rc;

CATBaseUnknown_var spFirstInstance = NULL_var;
spFirstInstance = pUDFInst_Ref->GetInstantiated(pUDFInst_Ref);
if (NULL_var == spFirstInstance) return 1;

//_spUDFInstance = spBeadUDFInstance = spFirstInstance;
//spUDFInstance = spFirstInstance;

if (!spFirstInstance)
return E_FAIL;

pudfInfo->spUDFInstance = spFirstInstance; //Store UDF instance
CATUnicodeString NewName = MRFGenericUtils::GetAliasName(spFirstInstance);

//rc = pUDFInst_Ref->SetDisplayName(NewName);

if (FAILED(rc)) return 1;

// 5-5 Ends the current instantiation
// ----------------------------------

rc = pUDFInst_Ref->EndInstantiate();
if (FAILED(rc)) return 1;

// List not any longer needed
delete pListOfInputRole;
pListOfInputRole = NULL;


CATIUseEntity *pUseEntityOnFirstInstance = NULL;
rc = spFirstInstance->QueryInterface(IID_CATIUseEntity, (void**)& pUseEntityOnFirstInstance);
if (SUCCEEDED(rc))
{
rc = DataCommonProtocolServices::Update(pUseEntityOnFirstInstance);
pUseEntityOnFirstInstance->Release(); pUseEntityOnFirstInstance = NULL;
}

Bag.RemoveRoot(piPLMCompOnFeatRef);

piPLMCompOnFeatRef->Release();
piPLMCompOnFeatRef = NULL;

MRFGenericUtils::ChangeMechanicalFather(spFirstInstance, spMechFeatOnPart);
CATPrtUpdateCom *pUpdateCommand_udf = new CATPrtUpdateCom(spFirstInstance, 0, 0);

if (spFirstInstance)
pudfInfo->listOutputFeat = this->GetUDFOutputInfo(spFirstInstance);


}
else
return rc;
}
}

return rc;
}
