// ObjexxFCL Headers
#include <ObjexxFCL/FArray.functions.hh>
#include <ObjexxFCL/FArray1D.hh>
#include <ObjexxFCL/MArray.functions.hh>

// EnergyPlus Headers
#include <CostEstimateManager.hh>
#include <DataCostEstimate.hh>
#include <DataDaylighting.hh>
#include <DataGlobals.hh>
#include <DataHeatBalance.hh>
#include <DataIPShortCuts.hh>
#include <DataPhotovoltaics.hh>
#include <DataPrecisionGlobals.hh>
#include <DataSurfaces.hh>
#include <DXCoils.hh>
#include <HeatingCoils.hh>
#include <InputProcessor.hh>
#include <PlantChillers.hh>
#include <UtilityRoutines.hh>

namespace EnergyPlus {

namespace CostEstimateManager {

	// Module containing the routines dealing with the Cost Estimation capability of EnergyPlus

	// MODULE INFORMATION:
	//       AUTHOR         B. Griffith
	//       DATE WRITTEN   April-May 2004
	//       MODIFIED       na
	//       RE-ENGINEERED  na

	// PURPOSE OF THIS MODULE:
	// produce a construction cost estimate report based on
	// input and certain building calculations by Energygplus

	// METHODOLOGY EMPLOYED:
	// Routine gets called once, Just before tabular reports.
	// Cost Estimate objects are child objects that will inherit from
	// other input objects.
	// Uses a Line Item methaphor where each Cost Estimate object is a line
	// Create report using utility subroutines taken from OutputReportTabular (by J.Glazer)

	// REFERENCES:
	// na

	// OTHER NOTES:
	// na

	// Using/Aliasing
	using namespace DataPrecisionGlobals;
	using DataGlobals::KickOffSimulation;
	using namespace DataCostEstimate;

	// Data
	// MODULE PARAMETER DEFINITIONS:
	// na

	// DERIVED TYPE DEFINITIONS:
	// na

	// MODULE VARIABLE DECLARATIONS:
	// na

	// SUBROUTINE SPECIFICATIONS FOR MODULE

	// Functions

	void
	SimCostEstimate()
	{

		// SUBROUTINE INFORMATION:
		//       AUTHOR         BGriffith
		//       DATE WRITTEN   April 2004
		//       MODIFIED       na
		//       RE-ENGINEERED  na

		// PURPOSE OF THIS SUBROUTINE:
		// Entry point; manage calls to other subroutines

		// METHODOLOGY EMPLOYED:
		// na

		// REFERENCES:
		// na

		// USE STATEMENTS:
		// na

		// Locals
		// SUBROUTINE ARGUMENT DEFINITIONS:
		// na

		// SUBROUTINE PARAMETER DEFINITIONS:
		// na

		// INTERFACE BLOCK SPECIFICATIONS:
		// na

		// DERIVED TYPE DEFINITIONS:
		// na

		// SUBROUTINE LOCAL VARIABLE DECLARATIONS:
		static bool GetCostInput( true );

		if ( GetCostInput ) {
			GetCostEstimateInput();
			GetCostInput = false;
		}

		// Need to add check Costs before this will work properly

		if ( KickOffSimulation ) return;

		if ( DoCostEstimate ) {

			CalcCostEstimate();

		}

	}

	void
	GetCostEstimateInput()
	{

		// SUBROUTINE INFORMATION:
		//       AUTHOR         BGriffith
		//       DATE WRITTEN   April 2004
		//       MODIFIED       na
		//       RE-ENGINEERED  na

		// PURPOSE OF THIS SUBROUTINE:
		// Get Cost Estimation object input.

		// METHODOLOGY EMPLOYED:
		// na

		// REFERENCES:
		// na

		// Using/Aliasing
		using InputProcessor::GetNumObjectsFound; // might also use FindItemInList
		using InputProcessor::GetObjectItem;
		using namespace DataIPShortCuts;

		// Locals
		// SUBROUTINE ARGUMENT DEFINITIONS:
		// na

		// SUBROUTINE PARAMETER DEFINITIONS:
		// na

		// INTERFACE BLOCK SPECIFICATIONS:
		// na

		// DERIVED TYPE DEFINITIONS:
		// na

		// SUBROUTINE LOCAL VARIABLE DECLARATIONS:
		int Item; // Item to be "gotten"
		int NumCostAdjust;
		int NumRefAdjust;
		int NumAlphas; // Number of Alphas for each GetObjectItem call
		int NumNumbers; // Number of Numbers for each GetObjectItem call
		int IOStatus; // Used in GetObjectItem
		static bool ErrorsFound( false ); // Set to true if errors in input, fatal at end of routine

		NumLineItems = GetNumObjectsFound( "ComponentCost:LineItem" );

		if ( NumLineItems == 0 ) {
			DoCostEstimate = false;
			return;
		} else {
			DoCostEstimate = true;
			//    WriteTabularFiles = .TRUE.
		}

		if ( ! allocated( CostLineItem ) ) {
			CostLineItem.allocate( NumLineItems );
		}

		cCurrentModuleObject = "ComponentCost:LineItem";

		for ( Item = 1; Item <= NumLineItems; ++Item ) {
			GetObjectItem( cCurrentModuleObject, Item, cAlphaArgs, NumAlphas, rNumericArgs, NumNumbers, IOStatus );
			CostLineItem( Item ).LineName = cAlphaArgs( 1 );
			CostLineItem( Item ).LineType = cAlphaArgs( 2 );
			CostLineItem( Item ).ParentObjType = cAlphaArgs( 3 );
			CostLineItem( Item ).ParentObjName = cAlphaArgs( 4 );
			CostLineItem( Item ).ParentObjKey = cAlphaArgs( 5 );
			CostLineItem( Item ).PerEach = rNumericArgs( 1 );
			CostLineItem( Item ).PerSquareMeter = rNumericArgs( 2 );
			CostLineItem( Item ).PerKiloWattCap = rNumericArgs( 3 );
			CostLineItem( Item ).PerKWCapPerCOP = rNumericArgs( 4 );
			CostLineItem( Item ).PerCubicMeter = rNumericArgs( 5 );
			CostLineItem( Item ).PerCubMeterPerSec = rNumericArgs( 6 );
			CostLineItem( Item ).PerUAinWattperDelK = rNumericArgs( 7 );
			CostLineItem( Item ).Qty = rNumericArgs( 8 );
			//    CostLineItem(item)%AnnualMaintFract   = rNumericArgs(9)
			//    CostLineItem(item)%MinorOverhallFract = rNumericArgs(10)
			//    CostLineItem(item)%MinorOverhallYears = rNumericArgs(11)
			//    CostLineItem(item)%MajorOverhallFract = rNumericArgs(12)
			//    CostLineItem(item)%MajorOverhallYears = rNumericArgs(13)
			//    CostLineItem(item)%LifeYears          = rNumericArgs(14)
			//    CostLineItem(item)%ValueAtReplacement = rNumericArgs(15)
		}

		//most input error checking to be performed later within Case construct in Calc routine.
		// do inits that aren't in a derived type
		NumCostAdjust = 0;
		NumRefAdjust = 0;

		cCurrentModuleObject = "ComponentCost:Adjustments";
		NumCostAdjust = GetNumObjectsFound( cCurrentModuleObject );
		if ( NumCostAdjust == 1 ) {
			GetObjectItem( cCurrentModuleObject, 1, cAlphaArgs, NumAlphas, rNumericArgs, NumNumbers, IOStatus );
			CurntBldg.MiscCostperSqMeter = rNumericArgs( 1 );
			CurntBldg.DesignFeeFrac = rNumericArgs( 2 );
			CurntBldg.ContractorFeeFrac = rNumericArgs( 3 );
			CurntBldg.ContingencyFrac = rNumericArgs( 4 );
			CurntBldg.BondCostFrac = rNumericArgs( 5 );
			CurntBldg.CommissioningFrac = rNumericArgs( 6 );
			CurntBldg.RegionalModifier = rNumericArgs( 7 );

		} else if ( NumCostAdjust > 1 ) {
			ShowSevereError( cCurrentModuleObject + ": Only one instance of this object is allowed." );
			ErrorsFound = true;
		}

		cCurrentModuleObject = "ComponentCost:Reference";
		NumRefAdjust = GetNumObjectsFound( cCurrentModuleObject );
		if ( NumRefAdjust == 1 ) {
			GetObjectItem( cCurrentModuleObject, 1, cAlphaArgs, NumAlphas, rNumericArgs, NumNumbers, IOStatus );
			RefrncBldg.LineItemTot = rNumericArgs( 1 );
			RefrncBldg.MiscCostperSqMeter = rNumericArgs( 2 );
			RefrncBldg.DesignFeeFrac = rNumericArgs( 3 );
			RefrncBldg.ContractorFeeFrac = rNumericArgs( 4 );
			RefrncBldg.ContingencyFrac = rNumericArgs( 5 );
			RefrncBldg.BondCostFrac = rNumericArgs( 6 );
			RefrncBldg.CommissioningFrac = rNumericArgs( 7 );
			RefrncBldg.RegionalModifier = rNumericArgs( 8 );

		} else if ( NumRefAdjust > 1 ) {
			ShowSevereError( cCurrentModuleObject + " : Only one instance of this object is allowed." );
			ErrorsFound = true;
		}

		if ( ErrorsFound ) {
			ShowFatalError( "Errors found in processing cost estimate input" );
		}

		CheckCostEstimateInput( ErrorsFound );

		if ( ErrorsFound ) {
			ShowFatalError( "Errors found in processing cost estimate input" );
		}

	}

	void
	CheckCostEstimateInput( bool & ErrorsFound ) // Set to true if errors in input, fatal at end of routine
	{

		// SUBROUTINE INFORMATION:
		//       AUTHOR         BGriffith
		//       DATE WRITTEN   April 2004
		//       MODIFIED       February 2005, M. J. Witte
		//                        Add subscript to DX coil variables due to new multimode DX coil
		//       RE-ENGINEERED  na

		// PURPOSE OF THIS SUBROUTINE:
		// Calculates the Cost Estimate based on inputs.

		// METHODOLOGY EMPLOYED:
		// na

		// REFERENCES:
		// na

		// Using/Aliasing
		using DataSurfaces::Surface;
		using DataSurfaces::TotSurfaces;
		using DataGlobals::NumOfZones;
		using DataHeatBalance::Construct;
		using DataHeatBalance::TotConstructs;
		using DataHeatBalance::Lights;
		using DataHeatBalance::Zone;
		using DataHeatBalance::TotLights;
		using InputProcessor::FindItem;
		using DXCoils::DXCoil;
		using DXCoils::NumDXCoils;
		using PlantChillers::ElectricChiller;
		using PlantChillers::ElectricChillerSpecs;
		using PlantChillers::NumElectricChillers;
		using DataPhotovoltaics::PVarray;
		using DataPhotovoltaics::NumPVs;
		using DataPhotovoltaics::NumSimplePVModuleTypes;
		using DataPhotovoltaics::iSimplePVModel;
		using namespace DataDaylighting;
		using HeatingCoils::HeatingCoil;
		using HeatingCoils::NumHeatingCoils;

		// Locals
		// SUBROUTINE ARGUMENT DEFINITIONS:

		// SUBROUTINE PARAMETER DEFINITIONS:
		// na

		// INTERFACE BLOCK SPECIFICATIONS:
		// na

		// DERIVED TYPE DEFINITIONS:
		// na

		// SUBROUTINE LOCAL VARIABLE DECLARATIONS:
		int Item; // do-loop counter for line items
		int ThisConstructID; // hold result of FindItem searching for Construct name
		int ThisSurfID; // hold result from findItem
		int ThisZoneID; // hold result from findItem

		std::string ThisConstructStr;

		//  LOGICAL,ALLOCATABLE, DIMENSION(:) :: uniqueSurfMask !
		//  REAL(r64), ALLOCATABLE, DIMENSION(:)   :: SurfMultipleARR
		//  INTEGER             :: surf ! do-loop counter for checking for surfaces for uniqueness
		int thisCoil; // index of named coil in its derived type
		bool WildcardObjNames;
		int thisChil;
		int thisPV;
		Real64 Multipliers;

		//Setup working data structure for line items
		for ( Item = 1; Item <= NumLineItems; ++Item ) { //Loop thru cost line items

			CostLineItem( Item ).LineNumber = Item;

			{ auto const SELECT_CASE_var( CostLineItem( Item ).ParentObjType );

			if ( SELECT_CASE_var == "GENERAL" ) {

			} else if ( SELECT_CASE_var == "CONSTRUCTION" ) {

				//test input for problems
				//  is PerSquareMeter non-zero? if it is are other cost per values set?
				//   issue warning that 'Cost Estimate requested for Constructions with zero cost per unit area
				if ( CostLineItem( Item ).PerSquareMeter == 0 ) {
					ShowSevereError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\" Construction object needs non-zero construction costs per square meter" );
					ErrorsFound = true;
				}

				ThisConstructStr = CostLineItem( Item ).ParentObjName;
				ThisConstructID = FindItem( ThisConstructStr, Construct.Name(), TotConstructs );
				if ( ThisConstructID == 0 ) { // do any surfaces have the specified construction? If not issue warning.
					ShowWarningError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\" Construction=\"" + CostLineItem( Item ).ParentObjName + "\", no surfaces have the Construction specified" );
					ShowContinueError( "No costs will be calculated for this Construction." );
					//        ErrorsFound = .TRUE.
					continue;
				}

			} else if ( ( SELECT_CASE_var == "COIL:DX" ) || ( SELECT_CASE_var == "COIL:COOLING:DX:SINGLESPEED" ) ) {
				WildcardObjNames = false;
				thisCoil = 0;
				// test if too many pricing methods are set in user input
				if ( ( CostLineItem( Item ).PerKiloWattCap > 0.0 ) && ( CostLineItem( Item ).PerEach > 0.0 ) ) {
					ShowSevereError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\", Coil:DX, too many pricing methods specified" );
					ErrorsFound = true;
				}
				if ( ( CostLineItem( Item ).PerKiloWattCap > 0.0 ) && ( CostLineItem( Item ).PerKWCapPerCOP > 0.0 ) ) {
					ShowSevereError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\", Coil:DX, too many pricing methods specified" );
					ErrorsFound = true;
				}
				if ( ( CostLineItem( Item ).PerEach > 0.0 ) && ( CostLineItem( Item ).PerKWCapPerCOP > 0.0 ) ) {
					ShowSevereError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\", Coil:DX, too many pricing methods specified" );
					ErrorsFound = true;
				}
				//  check for wildcard * in object name..
				if ( CostLineItem( Item ).ParentObjName == "*" ) { // wildcard, apply to all such components
					WildcardObjNames = true;

				} else if ( CostLineItem( Item ).ParentObjName == "" ) {
					ShowSevereError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\", Coil:DX: need to specify a Reference Object Name " );
					ErrorsFound = true;

				} else { // assume name is probably useful
					thisCoil = FindItem( CostLineItem( Item ).ParentObjName, DXCoil.Name(), NumDXCoils );
					if ( thisCoil == 0 ) {
						ShowWarningError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\", Coil:DX, invalid coil specified" );
						ShowContinueError( "Coil Specified=\"" + CostLineItem( Item ).ParentObjName + "\", calculations will not be completed for this item." );
					}
				}

			} else if ( SELECT_CASE_var == "COIL:HEATING:GAS" ) {

				WildcardObjNames = false;
				thisCoil = 0;
				// test if too many pricing methods are set in user input
				if ( ( CostLineItem( Item ).PerKiloWattCap > 0.0 ) && ( CostLineItem( Item ).PerEach > 0.0 ) ) {
					ShowSevereError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\", Coil:Heating:Gas, too many pricing methods specified" );
					ErrorsFound = true;
				}
				if ( ( CostLineItem( Item ).PerKiloWattCap > 0.0 ) && ( CostLineItem( Item ).PerKWCapPerCOP > 0.0 ) ) {
					ShowSevereError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\", Coil:Heating:Gas, too many pricing methods specified" );
					ErrorsFound = true;
				}
				if ( ( CostLineItem( Item ).PerEach > 0.0 ) && ( CostLineItem( Item ).PerKWCapPerCOP > 0.0 ) ) {
					ShowSevereError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\", Coil:Heating:Gas, too many pricing methods specified" );
					ErrorsFound = true;
				}
				//  check for wildcard * in object name..
				if ( CostLineItem( Item ).ParentObjName == "*" ) { // wildcard, apply to all such components
					WildcardObjNames = true;

				} else if ( CostLineItem( Item ).ParentObjName == "" ) {
					ShowSevereError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\", Coil:Heating:Gas, need to specify a Reference Object Name" );
					ErrorsFound = true;

				} else { // assume name is probably useful
					thisCoil = FindItem( CostLineItem( Item ).ParentObjName, HeatingCoil.Name(), NumHeatingCoils );
					if ( thisCoil == 0 ) {
						ShowWarningError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\", Coil:Heating:Gas, invalid coil specified" );
						ShowContinueError( "Coil Specified=\"" + CostLineItem( Item ).ParentObjName + "\", calculations will not be completed for this item." );
					}
				}

			} else if ( SELECT_CASE_var == "CHILLER:ELECTRIC" ) {
				if ( CostLineItem( Item ).ParentObjName == "" ) {
					ShowSevereError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\", Chiller:Electric, need to specify a Reference Object Name" );
					ErrorsFound = true;
				}

				thisChil = FindItem( CostLineItem( Item ).ParentObjName, ElectricChiller.ma( &ElectricChillerSpecs::Base ).Name(), NumElectricChillers );
				if ( thisChil == 0 ) {
					ShowWarningError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\", Chiller:Electric, invalid chiller specified." );
					ShowContinueError( "Chiller Specified=\"" + CostLineItem( Item ).ParentObjName + "\", calculations will not be completed for this item." );
				}

			} else if ( SELECT_CASE_var == "DAYLIGHTING:CONTROLS" ) {
				WildcardObjNames = false;

				if ( CostLineItem( Item ).ParentObjName == "*" ) { // wildcard, apply to all such components
					WildcardObjNames = true;
				} else if ( CostLineItem( Item ).ParentObjName == "" ) {
					ShowSevereError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\", Daylighting:Controls, need to specify a Reference Object Name" );
					ErrorsFound = true;
				} else {
					ThisZoneID = FindItem( CostLineItem( Item ).ParentObjName, Zone.Name(), NumOfZones );
					if ( ThisZoneID > 0 ) {
						CostLineItem( Item ).Qty = ZoneDaylight( ThisZoneID ).TotalDaylRefPoints;
					} else {
						ShowSevereError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\", Daylighting:Controls, need to specify a valid zone name" );
						ShowContinueError( "Zone specified=\"" + CostLineItem( Item ).ParentObjName + "\"." );
						ErrorsFound = true;
					}
				}

			} else if ( SELECT_CASE_var == "SHADING:ZONE:DETAILED" ) {
				if ( CostLineItem( Item ).ParentObjName != "" ) {
					ThisSurfID = FindItem( CostLineItem( Item ).ParentObjName, Surface.Name(), TotSurfaces );
					if ( ThisSurfID > 0 ) {
						ThisZoneID = FindItem( Surface( ThisSurfID ).ZoneName, Zone.Name(), NumOfZones );
						if ( ThisZoneID == 0 ) {
							ShowSevereError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\", Shading:Zone:Detailed, need to specify a valid zone name" );
							ShowContinueError( "Zone specified=\"" + Surface( ThisSurfID ).ZoneName + "\"." );
							ErrorsFound = true;
						}
					} else {
						ShowSevereError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\", Shading:Zone:Detailed, need to specify a valid surface name" );
						ShowContinueError( "Surface specified=\"" + CostLineItem( Item ).ParentObjName + "\"." );
						ErrorsFound = true;
					}
				} else {
					ShowSevereError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\", Shading:Zone:Detailed, specify a Reference Object Name" );
					ErrorsFound = true;
				}

			} else if ( SELECT_CASE_var == "LIGHTS" ) {

				if ( ( CostLineItem( Item ).PerKiloWattCap > 0.0 ) && ( CostLineItem( Item ).PerEach > 0.0 ) ) {
					ShowSevereError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\", Lights, too many pricing methods specified" );
					ErrorsFound = true;
				}

				if ( CostLineItem( Item ).PerKiloWattCap != 0.0 ) {
					if ( CostLineItem( Item ).ParentObjName != "" ) {
						ThisZoneID = FindItem( CostLineItem( Item ).ParentObjName, Zone.Name(), NumOfZones );
						if ( ThisZoneID == 0 ) {
							ShowSevereError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\", Lights, need to specify a valid zone name" );
							ShowContinueError( "Zone specified=\"" + CostLineItem( Item ).ParentObjName + "\"." );
							ErrorsFound = true;
						}
					} else {
						ShowSevereError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\", Lights, need to specify a Reference Object Name" );
						ErrorsFound = true;
					}
				}

			} else if ( SELECT_CASE_var == "GENERATOR:PHOTOVOLTAIC" ) {

				if ( CostLineItem( Item ).PerKiloWattCap != 0.0 ) {
					if ( CostLineItem( Item ).ParentObjName != "" ) {
						thisPV = FindItem( CostLineItem( Item ).ParentObjName, PVarray.Name(), NumPVs );
						if ( thisPV > 0 ) {
							ThisZoneID = FindItem( Surface( PVarray( thisPV ).SurfacePtr ).ZoneName, Zone.Name(), NumOfZones );
							if ( ThisZoneID == 0 ) {
								Multipliers = 1.0;
							} else {
								Multipliers = Zone( ThisZoneID ).Multiplier * Zone( ThisZoneID ).ListMultiplier;
							}
							if ( PVarray( thisPV ).PVModelType != iSimplePVModel ) {
								ShowSevereError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\", Generator:Photovoltaic, only available for model type PhotovoltaicPerformance:Simple" );
								ErrorsFound = true;
							}
						} else {
							ShowSevereError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\", Generator:Photovoltaic, need to specify a valid PV array" );
							ShowContinueError( "PV Array specified=\"" + CostLineItem( Item ).ParentObjName + "\"." );
							ErrorsFound = true;
						}
					} else {
						ShowSevereError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\", Generator:Photovoltaic, need to specify a Reference Object Name" );
						ErrorsFound = true;
					}
				} else {
					ShowSevereError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\", Generator:Photovoltaic, need to specify a per-kilowatt cost " );
					ErrorsFound = true;
				}

			} else {
				ShowWarningError( "ComponentCost:LineItem: \"" + CostLineItem( Item ).LineName + "\", invalid cost item -- not included in cost estimate." );
				ShowContinueError( "... invalid object type=" + CostLineItem( Item ).ParentObjType );

			}}

		}

	}

	void
	CalcCostEstimate()
	{

		// SUBROUTINE INFORMATION:
		//       AUTHOR         BGriffith
		//       DATE WRITTEN   April 2004
		//       MODIFIED       February 2005, M. J. Witte
		//                        Add subscript to DX coil variables due to new multimode DX coil
		//       RE-ENGINEERED  na

		// PURPOSE OF THIS SUBROUTINE:
		// Calculates the Cost Estimate based on inputs.

		// METHODOLOGY EMPLOYED:
		// na

		// REFERENCES:
		// na

		// Using/Aliasing
		using DataSurfaces::Surface;
		using DataSurfaces::TotSurfaces;
		using DataGlobals::NumOfZones;
		using DataHeatBalance::Construct;
		using DataHeatBalance::TotConstructs;
		using DataHeatBalance::Lights;
		using DataHeatBalance::Zone;
		using DataHeatBalance::TotLights;
		using InputProcessor::FindItem;
		using DXCoils::DXCoil;
		using DXCoils::NumDXCoils;
		using PlantChillers::ElectricChiller;
		using PlantChillers::ElectricChillerSpecs;
		using PlantChillers::NumElectricChillers;
		using DataPhotovoltaics::PVarray;
		using DataPhotovoltaics::NumPVs;
		using DataPhotovoltaics::NumSimplePVModuleTypes;
		using DataPhotovoltaics::iSimplePVModel;
		using namespace DataDaylighting;
		using HeatingCoils::HeatingCoil;
		using HeatingCoils::NumHeatingCoils;

		// Locals
		// SUBROUTINE ARGUMENT DEFINITIONS:
		// na

		// SUBROUTINE PARAMETER DEFINITIONS:
		// na

		// INTERFACE BLOCK SPECIFICATIONS:
		// na

		// DERIVED TYPE DEFINITIONS:
		// na

		// SUBROUTINE LOCAL VARIABLE DECLARATIONS:
		int Item; // do-loop counter for line items
		int ThisConstructID; // hold result of FindItem searching for Construct name
		int ThisSurfID; // hold result from findItem
		int ThisZoneID; // hold result from findItem

		std::string ThisConstructStr;

		FArray1D_bool uniqueSurfMask;
		FArray1D< Real64 > SurfMultipleARR;
		int surf; // do-loop counter for checking for surfaces for uniqueness
		int thisCoil; // index of named coil in its derived type
		bool WildcardObjNames;
		int thisChil;
		int thisPV;
		Real64 Multipliers;

		//Setup working data structure for line items
		for ( Item = 1; Item <= NumLineItems; ++Item ) { //Loop thru cost line items

			CostLineItem( Item ).LineNumber = Item;

			{ auto const SELECT_CASE_var( CostLineItem( Item ).ParentObjType );

			if ( SELECT_CASE_var == "GENERAL" ) {

				CostLineItem( Item ).Units = "Ea.";
				CostLineItem( Item ).ValuePer = CostLineItem( Item ).PerEach;
				CostLineItem( Item ).LineSubTotal = CostLineItem( Item ).Qty * CostLineItem( Item ).ValuePer;

			} else if ( SELECT_CASE_var == "CONSTRUCTION" ) {

				ThisConstructStr = CostLineItem( Item ).ParentObjName;
				ThisConstructID = FindItem( ThisConstructStr, Construct.Name(), TotConstructs );
				// need to determine unique surfacs... some surfaces are shared by zones and hence doubled
				uniqueSurfMask.dimension( TotSurfaces, true ); //init to true and change duplicates to false
				SurfMultipleARR.dimension( TotSurfaces, 1.0 );
				for ( surf = 1; surf <= TotSurfaces; ++surf ) {
					if ( Surface( surf ).ExtBoundCond >= 1 ) {
						if ( Surface( surf ).ExtBoundCond < surf ) { //already cycled through
							uniqueSurfMask( surf ) = false;
						}
					}
					if ( Surface( surf ).Construction == 0 ) { //throw out others for now
						uniqueSurfMask( surf ) = false;
					}
					if ( Surface( surf ).Zone > 0 ) {
						SurfMultipleARR( surf ) = Zone( Surface( surf ).Zone ).Multiplier * Zone( Surface( surf ).Zone ).ListMultiplier;

					}
				}
				//determine which surfaces have the construction type  and if any are duplicates..
				CostLineItem( Item ).Qty = sum( Surface.Area() * SurfMultipleARR, ( uniqueSurfMask && ( Surface.Construction() == ThisConstructID ) ) );
				CostLineItem( Item ).Units = "m2";
				CostLineItem( Item ).ValuePer = CostLineItem( Item ).PerSquareMeter;
				CostLineItem( Item ).LineSubTotal = CostLineItem( Item ).Qty * CostLineItem( Item ).ValuePer;

				uniqueSurfMask.deallocate();
				SurfMultipleARR.deallocate();

			} else if ( ( SELECT_CASE_var == "COIL:DX" ) || ( SELECT_CASE_var == "COIL:COOLING:DX:SINGLESPEED" ) ) {
				WildcardObjNames = false;
				thisCoil = 0;
				//  check for wildcard * in object name..
				if ( CostLineItem( Item ).ParentObjName == "*" ) { // wildcard, apply to all such components
					WildcardObjNames = true;
				} else if ( CostLineItem( Item ).ParentObjName != "" ) {
					thisCoil = FindItem( CostLineItem( Item ).ParentObjName, DXCoil.Name(), NumDXCoils );
				}

				if ( CostLineItem( Item ).PerKiloWattCap > 0.0 ) {
					if ( WildcardObjNames ) {
						CostLineItem( Item ).Qty = sum_col( DXCoil.RatedTotCap(), 1 ) / 1000.0;
						CostLineItem( Item ).Units = "kW (tot cool cap.)";
						CostLineItem( Item ).ValuePer = CostLineItem( Item ).PerKiloWattCap;
						CostLineItem( Item ).LineSubTotal = CostLineItem( Item ).Qty * CostLineItem( Item ).ValuePer;
					}
					if ( thisCoil > 0 ) {
						CostLineItem( Item ).Qty = DXCoil( thisCoil ).RatedTotCap( 1 ) / 1000.0;
						CostLineItem( Item ).Units = "kW (tot cool cap.)";
						CostLineItem( Item ).ValuePer = CostLineItem( Item ).PerKiloWattCap;
						CostLineItem( Item ).LineSubTotal = CostLineItem( Item ).Qty * CostLineItem( Item ).ValuePer;
					}
				}

				if ( CostLineItem( Item ).PerEach > 0.0 ) {
					if ( WildcardObjNames ) CostLineItem( Item ).Qty = double( NumDXCoils );
					if ( thisCoil > 0 ) CostLineItem( Item ).Qty = 1.0;
					CostLineItem( Item ).ValuePer = CostLineItem( Item ).PerEach;
					CostLineItem( Item ).LineSubTotal = CostLineItem( Item ).Qty * CostLineItem( Item ).ValuePer;
					CostLineItem( Item ).Units = "Ea.";
				}

				if ( CostLineItem( Item ).PerKWCapPerCOP > 0.0 ) {
					if ( WildcardObjNames ) {
						CostLineItem( Item ).Qty = sum_product_col( DXCoil.RatedCOP(), DXCoil.RatedTotCap(), 1 ) / 1000.0; //Autodesk:F2C++ Was sum( DXCoil.RatedCOP( 1 ) * DXCoil.RatedTotCap( 1 ) ) / 1000.0;
						CostLineItem( Item ).Units = "kW*COP (total, rated) ";
						CostLineItem( Item ).ValuePer = CostLineItem( Item ).PerKWCapPerCOP;
						CostLineItem( Item ).LineSubTotal = CostLineItem( Item ).Qty * CostLineItem( Item ).ValuePer;
					}
					if ( thisCoil > 0 ) {
						CostLineItem( Item ).Qty = DXCoil( thisCoil ).RatedCOP( 1 ) * DXCoil( thisCoil ).RatedTotCap( 1 ) / 1000.0;
						CostLineItem( Item ).Units = "kW*COP (total, rated) ";
						CostLineItem( Item ).ValuePer = CostLineItem( Item ).PerKWCapPerCOP;
						CostLineItem( Item ).LineSubTotal = CostLineItem( Item ).Qty * CostLineItem( Item ).ValuePer;
					}
				}

			} else if ( SELECT_CASE_var == "COIL:HEATING:GAS" ) {
				WildcardObjNames = false;
				thisCoil = 0;
				//  check for wildcard * in object name..
				if ( CostLineItem( Item ).ParentObjName == "*" ) { // wildcard, apply to all such components
					WildcardObjNames = true;
				} else if ( CostLineItem( Item ).ParentObjName != "" ) {
					thisCoil = FindItem( CostLineItem( Item ).ParentObjName, HeatingCoil.Name(), NumHeatingCoils );
				}

				if ( CostLineItem( Item ).PerKiloWattCap > 0.0 ) {
					if ( WildcardObjNames ) {

						CostLineItem( Item ).Qty = sum( HeatingCoil.NominalCapacity(), ( HeatingCoil.HCoilType_Num() == 1 ) ) / 1000.0;
						CostLineItem( Item ).Units = "kW (tot heat cap.)";
						CostLineItem( Item ).ValuePer = CostLineItem( Item ).PerKiloWattCap;
						CostLineItem( Item ).LineSubTotal = CostLineItem( Item ).Qty * CostLineItem( Item ).ValuePer;
					}
					if ( thisCoil > 0 ) {
						CostLineItem( Item ).Qty = HeatingCoil( thisCoil ).NominalCapacity / 1000.0;
						CostLineItem( Item ).Units = "kW (tot heat cap.)";
						CostLineItem( Item ).ValuePer = CostLineItem( Item ).PerKiloWattCap;
						CostLineItem( Item ).LineSubTotal = CostLineItem( Item ).Qty * CostLineItem( Item ).ValuePer;
					}
				}

				if ( CostLineItem( Item ).PerEach > 0.0 ) {
					if ( WildcardObjNames ) CostLineItem( Item ).Qty = NumHeatingCoils;
					if ( thisCoil > 0 ) CostLineItem( Item ).Qty = 1.0;
					CostLineItem( Item ).ValuePer = CostLineItem( Item ).PerEach;
					CostLineItem( Item ).LineSubTotal = CostLineItem( Item ).Qty * CostLineItem( Item ).ValuePer;
					CostLineItem( Item ).Units = "Ea.";
				}

				if ( CostLineItem( Item ).PerKWCapPerCOP > 0.0 ) {
					if ( WildcardObjNames ) {
						CostLineItem( Item ).Qty = sum( HeatingCoil.Efficiency() * HeatingCoil.NominalCapacity(), ( HeatingCoil.HCoilType_Num() == 1 ) ) / 1000.0;
						CostLineItem( Item ).Units = "kW*Eff (total, rated) ";
						CostLineItem( Item ).ValuePer = CostLineItem( Item ).PerKWCapPerCOP;
						CostLineItem( Item ).LineSubTotal = CostLineItem( Item ).Qty * CostLineItem( Item ).ValuePer;
					}
					if ( thisCoil > 0 ) {
						CostLineItem( Item ).Qty = HeatingCoil( thisCoil ).Efficiency * HeatingCoil( thisCoil ).NominalCapacity / 1000.0;
						CostLineItem( Item ).Units = "kW*Eff (total, rated) ";
						CostLineItem( Item ).ValuePer = CostLineItem( Item ).PerKWCapPerCOP;
						CostLineItem( Item ).LineSubTotal = CostLineItem( Item ).Qty * CostLineItem( Item ).ValuePer;
					}
				}

			} else if ( SELECT_CASE_var == "CHILLER:ELECTRIC" ) {
				thisChil = FindItem( CostLineItem( Item ).ParentObjName, ElectricChiller.ma( &ElectricChillerSpecs::Base ).Name(), NumElectricChillers );
				if ( ( thisChil > 0 ) && ( CostLineItem( Item ).PerKiloWattCap > 0.0 ) ) {
					CostLineItem( Item ).Qty = ElectricChiller( thisChil ).Base.NomCap / 1000.0;
					CostLineItem( Item ).Units = "kW (tot cool cap.)";
					CostLineItem( Item ).ValuePer = CostLineItem( Item ).PerKiloWattCap;
					CostLineItem( Item ).LineSubTotal = CostLineItem( Item ).Qty * CostLineItem( Item ).ValuePer;
				}
				if ( ( thisChil > 0 ) && ( CostLineItem( Item ).PerKWCapPerCOP > 0.0 ) ) {
					CostLineItem( Item ).Qty = ElectricChiller( thisChil ).Base.COP * ElectricChiller( thisChil ).Base.NomCap / 1000.0;
					CostLineItem( Item ).Units = "kW*COP (total, rated) ";
					CostLineItem( Item ).ValuePer = CostLineItem( Item ).PerKWCapPerCOP;
					CostLineItem( Item ).LineSubTotal = CostLineItem( Item ).Qty * CostLineItem( Item ).ValuePer;
				}
				if ( ( thisChil > 0 ) && ( CostLineItem( Item ).PerEach > 0.0 ) ) {
					CostLineItem( Item ).Qty = 1.0;
					CostLineItem( Item ).Units = "Ea.";
					CostLineItem( Item ).ValuePer = CostLineItem( Item ).PerEach;
					CostLineItem( Item ).LineSubTotal = CostLineItem( Item ).Qty * CostLineItem( Item ).ValuePer;
				}

			} else if ( SELECT_CASE_var == "DAYLIGHTING:CONTROLS" ) {
				WildcardObjNames = false;

				if ( CostLineItem( Item ).ParentObjName == "*" ) { // wildcard, apply to all such components
					WildcardObjNames = true;
					CostLineItem( Item ).Qty = sum( ZoneDaylight.TotalDaylRefPoints() );
				} else if ( CostLineItem( Item ).ParentObjName != "" ) {
					ThisZoneID = FindItem( CostLineItem( Item ).ParentObjName, Zone.Name(), NumOfZones );
					if ( ThisZoneID > 0 ) {
						CostLineItem( Item ).Qty = ZoneDaylight( ThisZoneID ).TotalDaylRefPoints;
					}
				}

				CostLineItem( Item ).Units = "Ea.";
				CostLineItem( Item ).ValuePer = CostLineItem( Item ).PerEach;
				CostLineItem( Item ).LineSubTotal = CostLineItem( Item ).Qty * CostLineItem( Item ).ValuePer;

			} else if ( SELECT_CASE_var == "SHADING:ZONE:DETAILED" ) {
				if ( CostLineItem( Item ).ParentObjName != "" ) {
					ThisSurfID = FindItem( CostLineItem( Item ).ParentObjName, Surface.Name(), TotSurfaces );
					if ( ThisSurfID > 0 ) {
						ThisZoneID = FindItem( Surface( ThisSurfID ).ZoneName, Zone.Name(), NumOfZones );
						if ( ThisZoneID > 0 ) {
							CostLineItem( Item ).Qty = Surface( ThisSurfID ).Area * Zone( ThisZoneID ).Multiplier * Zone( ThisZoneID ).ListMultiplier;
							CostLineItem( Item ).Units = "m2";
							CostLineItem( Item ).ValuePer = CostLineItem( Item ).PerSquareMeter;
							CostLineItem( Item ).LineSubTotal = CostLineItem( Item ).Qty * CostLineItem( Item ).ValuePer;
						}
					}
				}

			} else if ( SELECT_CASE_var == "LIGHTS" ) {

				if ( CostLineItem( Item ).PerEach != 0.0 ) {
					CostLineItem( Item ).Qty = 1.0;
					CostLineItem( Item ).Units = "Ea.";
					CostLineItem( Item ).ValuePer = CostLineItem( Item ).PerEach;
					CostLineItem( Item ).LineSubTotal = CostLineItem( Item ).Qty * CostLineItem( Item ).ValuePer;
				}

				if ( CostLineItem( Item ).PerKiloWattCap != 0.0 ) {
					if ( CostLineItem( Item ).ParentObjName != "" ) {
						ThisZoneID = FindItem( CostLineItem( Item ).ParentObjName, Zone.Name(), NumOfZones );
						if ( ThisZoneID > 0 ) {
							CostLineItem( Item ).Qty = ( Zone( ThisZoneID ).Multiplier * Zone( ThisZoneID ).ListMultiplier / 1000.0 ) * sum( Lights.DesignLevel(), Lights.ZonePtr() == ThisZoneID ); //this handles more than one light object per zone.
							CostLineItem( Item ).Units = "kW";
							CostLineItem( Item ).ValuePer = CostLineItem( Item ).PerKiloWattCap;
							CostLineItem( Item ).LineSubTotal = CostLineItem( Item ).Qty * CostLineItem( Item ).ValuePer;
						}
					}
				}

			} else if ( SELECT_CASE_var == "GENERATOR:PHOTOVOLTAIC" ) {

				if ( CostLineItem( Item ).PerKiloWattCap != 0.0 ) {
					if ( CostLineItem( Item ).ParentObjName != "" ) {
						thisPV = FindItem( CostLineItem( Item ).ParentObjName, PVarray.Name(), NumPVs );
						if ( thisPV > 0 ) {
							ThisZoneID = FindItem( Surface( PVarray( thisPV ).SurfacePtr ).ZoneName, Zone.Name(), NumOfZones );
							if ( ThisZoneID == 0 ) {
								Multipliers = 1.0;
							} else {
								Multipliers = Zone( ThisZoneID ).Multiplier * Zone( ThisZoneID ).ListMultiplier;
							}
							if ( PVarray( thisPV ).PVModelType == iSimplePVModel ) {
								CostLineItem( Item ).Qty = 1000.0 * PVarray( thisPV ).SimplePVModule.AreaCol * PVarray( thisPV ).SimplePVModule.PVEfficiency * Multipliers / 1000.0;

							}
							CostLineItem( Item ).Units = "kW (rated)";
							CostLineItem( Item ).ValuePer = CostLineItem( Item ).PerKiloWattCap;
							CostLineItem( Item ).LineSubTotal = CostLineItem( Item ).Qty * CostLineItem( Item ).ValuePer;
						}
					}
				}

			}}

		}

		//now sum up the line items, result for the current building

		CurntBldg.LineItemTot = sum( CostLineItem.LineSubTotal() );

	}

	//     NOTICE

	//     Copyright � 1996-2014 The Board of Trustees of the University of Illinois
	//     and The Regents of the University of California through Ernest Orlando Lawrence
	//     Berkeley National Laboratory.  All rights reserved.

	//     Portions of the EnergyPlus software package have been developed and copyrighted
	//     by other individuals, companies and institutions.  These portions have been
	//     incorporated into the EnergyPlus software package under license.   For a complete
	//     list of contributors, see "Notice" located in main.cc.

	//     NOTICE: The U.S. Government is granted for itself and others acting on its
	//     behalf a paid-up, nonexclusive, irrevocable, worldwide license in this data to
	//     reproduce, prepare derivative works, and perform publicly and display publicly.
	//     Beginning five (5) years after permission to assert copyright is granted,
	//     subject to two possible five year renewals, the U.S. Government is granted for
	//     itself and others acting on its behalf a paid-up, non-exclusive, irrevocable
	//     worldwide license in this data to reproduce, prepare derivative works,
	//     distribute copies to the public, perform publicly and display publicly, and to
	//     permit others to do so.

	//     TRADEMARKS: EnergyPlus is a trademark of the US Department of Energy.

} // CostEstimateManager

} // EnergyPlus
