import React from "react";
import { Link, useLocation } from "react-router-dom";
import { createPageUrl } from "@/utils";
import { Waves, BarChart3, Shield, FileText, Gauge, Wrench, Settings } from "lucide-react";

const navigationItems = [
  {
    title: "Aguada",
    url: createPageUrl("Aguada"),
    icon: Waves,
  },
  {
    title: "Consumo",
    url: createPageUrl("Consumo"),
    icon: BarChart3,
  },
  {
    title: "Medições",
    url: createPageUrl("Medicoes"),
    icon: Gauge,
  },
  {
    title: "Rede CAV",
    url: createPageUrl("CAV"),
    icon: Shield,
  },
  {
    title: "Manutenção",
    url: createPageUrl("Manutencao"),
    icon: Wrench,
  },
  {
    title: "Registros",
    url: createPageUrl("Registros"),
    icon: FileText,
  },
   {
    title: "Configurações",
    url: createPageUrl("Configuracoes"),
    icon: Settings,
  },
];

export default function Layout({ children, currentPageName }) {
  const location = useLocation();

  return (
    <div className="min-h-screen bg-gray-50 text-gray-800 font-sans flex flex-col">
      <div className="fixed inset-0 w-full h-full bg-grid-gray-200/40 [mask-image:radial-gradient(100%_100%_at_top_right,white,transparent)] -z-10"></div>
      
      {/* Topbar */}
      <header className="sticky top-0 z-50 bg-white/70 backdrop-blur-lg border-b border-gray-200 shadow-sm">
        <div className="container mx-auto px-6">
          <div className="flex items-center justify-between h-16">
            {/* Logo */}
            <div className="flex items-center gap-3">
              <div className="bg-sky-100 p-2 rounded-lg border border-sky-200">
                <Waves className="w-5 h-5 text-sky-600" />
              </div>
              <div>
                <h1 className="text-lg font-bold text-gray-800">CMASM</h1>
                <p className="text-xs text-gray-600 -mt-1">Sistema Aguada</p>
              </div>
            </div>

            {/* Navigation */}
            <nav className="hidden md:flex items-center gap-1">
              {navigationItems.map((item) => {
                const isActive = location.pathname === item.url;
                return (
                  <Link
                    key={item.title}
                    to={item.url}
                    className={`
                      flex items-center gap-2 px-3 py-2 rounded-md transition-all duration-200 text-sm font-medium
                      ${isActive 
                        ? 'bg-sky-100 text-sky-700 shadow-sm' 
                        : 'text-gray-600 hover:bg-gray-100 hover:text-gray-900'
                      }
                    `}
                  >
                    <item.icon className="w-4 h-4" />
                    <span>{item.title}</span>
                  </Link>
                );
              })}
            </nav>
            
            {/* Placeholder for User Profile */}
            <div className="w-10 h-10 bg-gray-200 rounded-full border border-gray-300"></div>
          </div>
        </div>
      </header>

      {/* Main Content */}
      <main className="flex-1 p-6">
        <div className="container mx-auto">
          {children}
        </div>
      </main>
    </div>
  );
}
