# Firmware Aguada - Projeto Reorganizado

## Nova Estrutura Proposta

```
aguada-telemetry/
├── firmware/                    # Todo código ESP32
│   ├── node_ultra1/
│   ├── node_ultra2/
│   ├── gateway_devkit_v1/
│   ├── components/             # Módulos compartilhados
│   ├── common/                 # Headers compartilhados
│   └── .old_versions/          # Arquivos de referência (opcional)
│
├── backend/                    # Servidor PHP/Node.js
│   ├── api/                    # Endpoints REST
│   ├── config/                 # Configurações
│   └── database/               # Scripts SQL
│
├── frontend/                   # Interface web
│   ├── public/
│   ├── src/
│   └── package.json
│
├── database/                   # Schemas e migrations
│   ├── migrations/
│   └── schema.sql
│
├── docs/                       # Documentação
│   ├── architecture.md
│   └── api-reference.md
│
├── .gitignore                  # Ignorar builds
├── README.md
└── docker-compose.yml          # Para backend + database
```

## Ações Imediatas (Liberar Espaço)

### 1. Limpar builds (recupera ~1.5GB)
```bash
# Remover builds do firmware ativo
cd ~/firmware_aguada
rm -rf node_ultra1/build node_ultra2/build gateway_devkit_v1/build

# Remover pasta de referência antiga (opcional, mas libera 1GB)
rm -rf .REF__firmware
```

### 2. Criar .gitignore
```
# ESP-IDF builds
**/build/
**/sdkconfig.old
**/.ninja_*

# PlatformIO
.pio/
.vscode/.browse.c_cpp.db*
.vscode/c_cpp_properties.json
.vscode/launch.json

# Node/Frontend
node_modules/
dist/
.env

# IDE
.vscode/*
!.vscode/settings.json
.idea/

# Logs
*.log
```

### 3. Reorganizar (sem mover arquivos ainda)
```bash
# Criar estrutura base
mkdir -p backend/{api,config,database}
mkdir -p frontend/src
mkdir -p database/migrations
mkdir -p docs

# Mover backend atual
mv backend_xampp/* backend/
```

## Próximos Passos

1. **Backup antes de reorganizar**: `tar -czf aguada-backup-$(date +%F).tar.gz firmware_aguada/`
2. **Limpar builds**: Executar comandos acima
3. **Mover arquivos**: Seguir nova estrutura
4. **Atualizar paths**: Ajustar referências no código
5. **Testar build**: Verificar que firmware ainda compila

## Benefícios

- ✅ **1.5GB+ liberados** (sem builds versionados)
- ✅ **Separação clara** de firmware/backend/frontend
- ✅ **Mais rápido** para VS Code indexar
- ✅ **Melhor para Git** (builds não versionados)
- ✅ **Escalável** para adicionar frontend
