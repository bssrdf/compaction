
#include <cmath>

#include "config.h"
#include "surfacedensity.h"
#include "meteo.h"
#include "logging.h"
#include "constants.h"

namespace DSM{ 

std::unique_ptr<SurfaceDensity> instantiate_surfacedensity(Meteo& meteo){
   const char * option_name = "fresh_snow_density:which_fsd";
   int which_fsd = config.getInt(option_name, false, 0, 6, 1);

   switch (which_fsd) {
      case 0   : return { std::make_unique<SurfaceDensityConstant>(meteo) };
      case 1   : return { std::make_unique<SurfaceDensityHelsen2008>(meteo) };
      case 2   : return { std::make_unique<SurfaceDensityLenaerts2012>(meteo) };
      case 3   : return { std::make_unique<SurfaceDensityCROCUS>(meteo) };
      case 4   : return { std::make_unique<SurfaceDensityAnderson>(meteo) };
      case 5   : return { std::make_unique<SurfaceDensityAndersonListon>(meteo) };
      case 6   : return { std::make_unique<SurfaceDensitySlater2016>(meteo) };
      default:
         logger << "ERROR: unknown value: " << which_fsd << " for config option " << option_name << std::endl;
         std::abort();
   }
}

SurfaceDensity::SurfaceDensity(Meteo& meteo) : _meteo(meteo) { } 

SurfaceDensityConstant::SurfaceDensityConstant(Meteo& meteo) : SurfaceDensity(meteo) { 
   const char * option_name = "fresh_snow_density:density";
   _val = config.getDouble(option_name, true, 1., 1000., -1.);
} 

SurfaceDensityHelsen2008::SurfaceDensityHelsen2008(Meteo& meteo) : SurfaceDensity(meteo) { 
   logger << "SurfaceDensityHelsen2008()" << std::endl; 
}

SurfaceDensityLenaerts2012::SurfaceDensityLenaerts2012(Meteo& meteo) : SurfaceDensity(meteo) { 
   logger << "SurfaceDensityLenaerts2012()" << std::endl; 
}

SurfaceDensityCROCUS::SurfaceDensityCROCUS(Meteo& meteo) : SurfaceDensity(meteo) { 
   logger << "SurfaceDensityCROCUS()" << std::endl; 
}

SurfaceDensityAnderson::SurfaceDensityAnderson(Meteo& meteo) : SurfaceDensity(meteo) { 
   logger << "SurfaceDensityAnderson()" << std::endl; 
}

SurfaceDensityAndersonListon::SurfaceDensityAndersonListon(Meteo& meteo) : SurfaceDensity(meteo) { 
   _sda = std::make_unique<SurfaceDensityAnderson>(meteo);
   logger << "SurfaceDensityAndersonListon()" << std::endl; 
}

SurfaceDensitySlater2016::SurfaceDensitySlater2016(Meteo& meteo) : SurfaceDensity(meteo) { 
   logger << "SurfaceDensitySlater2016()" << std::endl; 
}


double SurfaceDensityConstant::density() {
   return _val;
}

double SurfaceDensityHelsen2008::density() {
   /* after Helsen (2008) */
   return -154.91+1.4266*(73.6+1.06*_meteo.annualTskin()+0.0669*_meteo.annualAcc()+4.77*_meteo.annualW10m());
}

double SurfaceDensityLenaerts2012::density() {
   /* Lenaerts et al. 2012 
      formula 11
      The multiple linear regression that relates fresh snow density to mean surface temperature (Tsfc,Acc) and 10 m wind speed (U10m,Acc) during accumulation
      */
   static const double a = 97.5;
   static const double b = 0.77; 
   static const double c = 4.49;
   return a + b*_meteo.surfaceTemperature() + c*_meteo.surfaceWind();
}

double SurfaceDensityCROCUS::density() {
   static const double a = 109.;
   static const double b = 6.;
   static const double c = 26.;
   const double rho =  a + b*(_meteo.surfaceTemperature()-T0) + c*sqrt(_meteo.surfaceWind());
   return std::max(rho, 50.);
}

double SurfaceDensityAnderson::density() {
   /* Anderson 1976 */
   const double Tskin = _meteo.surfaceTemperature();
   double dens;
   if (Tskin > T0+2) {
      dens = 50. + 1.7*pow(17.,1.5);
   } else if (T0-15. < Tskin) {
      dens = 50. + 1.7*pow(Tskin-T0+15, 1.5);
   } else {
      dens = 50.;
   }
   return dens;
}

double SurfaceDensityAndersonListon::density() {
   /* CLM4.5 uses Anderson 1976 for temperature dependence and Liston et al 2007 for 
      wind dependence.
      Glen Liston et al, Simulating complex snow distributions in windy environments using SnowTran-3D
      Journal of Glaciology, Vol. 53, No. 181, 2007
   */
   const double forc_wind = _meteo.surfaceWind();
   double dens = _sda->density(); // Anderson density
   if (forc_wind >= 5.) {
      dens += 25. + 250. * (1. - exp(-0.2 * (forc_wind - 5.)));
   }
   return dens;
}

double SurfaceDensitySlater2016::density(){
   /*       
       ! Andrew Slater: A temp of about -15C gives the nicest
       ! "blower" powder, but as you get colder the flake size decreases so
       ! density goes up. e.g. the smaller snow crystals from the Arctic and Antarctic
       ! winters
   */
   const double forc_wind = _meteo.surfaceWind();
   const double Tskin = _meteo.surfaceTemperature();
   double dens;

   if (Tskin > T0+2) {
      dens = 170.;
   } else if (T0-15. < Tskin) {
      dens = 50. + 1.7*pow(Tskin-T0+15, 1.5);
   } else {
      dens = -3.8333*(Tskin-T0) - 0.0333*pow(Tskin-T0, 2);
   }
 
   /*
   ! Density offset for wind-driven compaction, initial ideas based on Liston et. al (2007) J. Glaciology,
   ! 53(181), 241-255. Modified for a continuous wind impact and slightly more sensitive
   ! to wind - Andrew Slater, 2016
   */
   dens = dens + 266.861 * pow((1. + std::tanh(forc_wind/5.0))/2., 8.8);
   
   return dens;
}

} // namespace
