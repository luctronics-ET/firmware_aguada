#!/bin/bash
# Script de verificação de recursos das páginas

BASE_URL="http://localhost:8080"

echo "=========================================="
echo "VERIFICAÇÃO DE RECURSOS - AGUADA FRONTEND"
echo "=========================================="
echo ""

# Função para verificar URL
check_url() {
    local url=$1
    local name=$2
    
    status=$(curl -s -o /dev/null -w "%{http_code}" "$url")
    
    if [ "$status" -eq 200 ]; then
        echo "✅ $name - OK ($status)"
    else
        echo "❌ $name - FALHOU ($status)"
    fi
}

echo "--- Páginas Principais ---"
check_url "$BASE_URL/mapa.html" "Mapa"
check_url "$BASE_URL/scada_new.html" "SCADA Nova"
check_url "$BASE_URL/consumo.html" "Consumo"
check_url "$BASE_URL/relatorios_lista.html" "Relatórios Lista"

echo ""
echo "--- Componentes ---"
check_url "$BASE_URL/components/navigation.html" "Navigation Component"
check_url "$BASE_URL/components/header.html" "Header Component"

echo ""
echo "--- Assets CSS ---"
check_url "$BASE_URL/assets/css/variables.css" "Variables CSS"
check_url "$BASE_URL/assets/css/animations.css" "Animations CSS"

echo ""
echo "--- CDN Externos ---"
check_url "https://cdn.tailwindcss.com" "Tailwind CSS CDN"
check_url "https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.js" "Chart.js CDN"
check_url "https://unpkg.com/leaflet@1.9.4/dist/leaflet.css" "Leaflet CSS CDN"
check_url "https://unpkg.com/leaflet@1.9.4/dist/leaflet.js" "Leaflet JS CDN"

echo ""
echo "--- Teste de Fetch JavaScript ---"
echo "Verificando se o browser consegue carregar componentes..."

# Simular carregamento via curl
echo ""
echo "📄 Conteúdo de navigation.html (primeiras 5 linhas):"
curl -s "$BASE_URL/components/navigation.html" | head -5

echo ""
echo "📄 Conteúdo de header.html (primeiras 5 linhas):"
curl -s "$BASE_URL/components/header.html" | head -5

echo ""
echo "=========================================="
echo "VERIFICAÇÃO COMPLETA!"
echo "=========================================="
