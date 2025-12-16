#!/bin/bash
# Script para limpar builds e liberar espaço

echo "=== Aguada Telemetry - Limpeza de Builds ==="
echo ""

# Verificar tamanho atual
echo "📊 Tamanho ANTES da limpeza:"
du -sh . 2>/dev/null
echo ""

# Confirmação
read -p "⚠️  Remover TODAS as pastas build/? (s/N): " -n 1 -r
echo ""
if [[ ! $REPLY =~ ^[Ss]$ ]]; then
    echo "❌ Operação cancelada"
    exit 1
fi

# Limpar builds ativos
echo "🧹 Removendo builds do firmware ativo..."
rm -rf node_ultra1/build
rm -rf node_ultra2/build
rm -rf gateway_devkit_v1/build
echo "✅ Builds ativos removidos"

# Opção para remover referências antigas
read -p "🗑️  Remover .REF__firmware/ também? (libera ~1GB) (s/N): " -n 1 -r
echo ""
if [[ $REPLY =~ ^[Ss]$ ]]; then
    rm -rf .REF__firmware
    echo "✅ Referências antigas removidas"
else
    echo "ℹ️  Mantendo .REF__firmware/"
fi

# Limpar outros arquivos temporários
echo "�� Removendo arquivos temporários..."
find . -name "*.pyc" -delete 2>/dev/null
find . -name "__pycache__" -type d -exec rm -rf {} + 2>/dev/null
find . -name ".DS_Store" -delete 2>/dev/null
echo "✅ Temporários removidos"

echo ""
echo "📊 Tamanho DEPOIS da limpeza:"
du -sh . 2>/dev/null
echo ""
echo "✨ Limpeza concluída!"
