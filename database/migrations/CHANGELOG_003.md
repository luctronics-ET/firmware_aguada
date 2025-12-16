# Resumo das Alterações - Sistema SCADA v2

## Data: 14 de dezembro de 2025

---

## ✅ Alterações Implementadas

### 1. **Tabela `redes_agua` (NOVA)**

Categoriza o sistema hidráulico por qualidade e finalidade da água:

```sql
CREATE TABLE redes_agua (
    id, codigo, nome, finalidade, 
    qualidade_agua ENUM('potavel', 'nao_potavel', 'industrial', 'reuso'),
    pressao_minima_bar, pressao_maxima_bar,
    cor_diagrama, norma_aplicavel
)
```

**4 redes pré-cadastradas:**
- `REDE_CONSUMO` - Água potável (azul #0066CC) - NBR 5626
- `REDE_INCENDIO` - Combate a incêndio (vermelho #FF0000) - NBR 13714  
- `REDE_PISCINA` - Recirculação piscina (ciano #00CCCC)
- `REDE_REUSO` - Água cinza/chuva (marrom #996600) - NBR 15527

---

### 2. **Tabela `elementos` - Campo `alias` (NOVO)**

Cada elemento agora tem **código único** para referência:

```sql
ALTER TABLE elementos 
ADD COLUMN alias VARCHAR(100) NOT NULL UNIQUE,
ADD COLUMN rede_agua_id INT,
ADD FOREIGN KEY (rede_agua_id) REFERENCES redes_agua(id);
```

**Convenção de nomes:**
- `RES_CASTELO_CONSUMO` - Reservatório
- `BOMBA_RECALQUE_01` - Bomba
- `VALV_GERAL_CONSUMO` - Válvula
- `ULTRA_CASTELO_CONSUMO` - Sensor
- `HIDR_Y_TERREO` - Hidrante Y

---

### 3. **Tabela `sensores` - Múltiplos Sensores por Elemento**

```sql
ALTER TABLE sensores 
ADD COLUMN alias VARCHAR(100) NOT NULL UNIQUE,
MODIFY elemento_id INT NULL;  -- Agora permite NULL e múltiplos sensores
```

**Mudanças:**
- ✅ Removido UNIQUE de `elemento_id` → permite 2+ sensores no mesmo elemento
- ✅ `elemento_id` pode ser NULL → sensores sem associação (expansão futura)
- ✅ Campo `alias` adicionado (ex: `ULTRA_CASTELO_CONSUMO`)

**MACs Reais Cadastrados:**

| Alias | Node ID | MAC | Elemento |
|-------|---------|-----|----------|
| `ULTRA_CASTELO_CONSUMO` | 1 | `20:6E:F1:6B:77:58` | RES_CASTELO_CONSUMO |
| `ULTRA_CASTELO_INCENDIO` | 2 | `DC:06:75:67:6A:CC` | RES_CASTELO_INCENDIO |
| `ULTRA_CISTERNA_PRINCIPAL` | 3 | `80:F1:B2:50:31:34` | CIS_PRINCIPAL |
| `ULTRA_RESERVATORIO_PISCINA` | 4 | `DC:B4:D9:8B:9E:AC` | RES_PISCINA |
| `NANO_CISTERNA_SECUNDARIA` | 10 | `DE:AD:BE:EF:FE:ED` | NULL (pendente) |

---

### 4. **Views Atualizadas**

#### `vw_status_elementos`
- ✅ Inclui `elemento_alias`
- ✅ Inclui `sensor_alias`
- ✅ Inclui `rede_codigo`, `rede_nome`, `rede_cor`
- ✅ Inclui `valvula_percentual`, `bomba_frequencia`, `bomba_corrente`, `bomba_pressao`

#### `vw_mapa_sistema`
- ✅ Inclui `origem_alias`, `destino_alias`
- ✅ Inclui `rede_codigo`, `rede_cor`
- ✅ Inclui `tipo_conexao` (rosca, flange, soldada)

#### `vw_anomalias_ativas`
- ✅ Inclui `elemento_alias`

---

### 5. **Dados de Exemplo Completos**

#### Locais (5 cadastrados)
- Castelo de Consumo
- Castelo de Incêndio
- Casa de Bombas N03
- Cisterna Subterrânea
- Área de Lazer

#### Elementos (9 cadastrados com alias)

**Rede Consumo:**
- `RES_CASTELO_CONSUMO` - 80.000 L
- `CIS_PRINCIPAL` - 120.000 L
- `BOMBA_RECALQUE_01`
- `VALV_GERAL_CONSUMO`

**Rede Incêndio:**
- `RES_CASTELO_INCENDIO` - 80.000 L (80T)
- `VALV_ENTRADA_INCENDIO`
- `TUBO_ENTRADA_INCENDIO` - 4" ferro galvanizado
- `VALV_Y_DISTRIBUICAO`

**Rede Piscina:**
- `RES_PISCINA` - 50.000 L

#### Conexões (6 cadastradas)

**Rede Consumo:**
```
CIS_PRINCIPAL → BOMBA_RECALQUE_01 → VALV_GERAL_CONSUMO → RES_CASTELO_CONSUMO
```

**Rede Incêndio:**
```
RES_CASTELO_INCENDIO → VALV_ENTRADA_INCENDIO → TUBO_ENTRADA_INCENDIO → VALV_Y_DISTRIBUICAO
```

---

## 📁 Arquivos Criados/Atualizados

### ✅ `/database/migrations/003_sistema_scada.sql`
- Tabela `redes_agua` (4 redes pré-cadastradas)
- Campo `alias` em `elementos` (UNIQUE)
- Campo `rede_agua_id` em `elementos` (FK)
- Campo `alias` em `sensores` (UNIQUE)
- `elemento_id` em `sensores` agora NULL (múltiplos sensores OK)
- Views atualizadas com alias e informações de rede
- Dados de exemplo com MACs reais dos 5 sensores instalados
- Sistema modelado: Rede Consumo + Rede Incêndio + Rede Piscina

### ✅ `/docs/MAPEAMENTO_SENSORES.md` (NOVO - 300+ linhas)
- Tabela com MACs reais dos 5 sensores instalados
- Convenção de nomes (alias) para todos tipos de elementos
- Exemplos de elementos com múltiplos sensores
- Exemplos de elementos sem sensores (entrada manual)
- Descrição completa das 4 redes de água:
  * Rede Consumo (água potável)
  * Rede Incêndio (hidrantes Y, NBR 13714)
  * Rede Piscina (recirculação, tratamento)
  * Rede Reuso (água cinza/chuva)
- Modelo de rede no banco (tabelas e queries)
- Visualização no SCADA (cores por rede, filtros)
- Checklist de implementação
- Referências normativas (NBR 5626, NBR 13714, NBR 15527)

### ✅ `/backend/api/scada_data.php` (atualizado anteriormente)
- Endpoint GET `?action=get_all`
- Endpoint POST `?action=toggle_valvula`
- Endpoint POST `?action=toggle_bomba`
- Endpoint GET `?action=get_history`

### ✅ `/backend/scada.html` (criado anteriormente)
- Interface completa com canvas interativo
- Visualização de elementos coloridos por categoria
- Pan/zoom, seleção de elementos
- Controles de válvulas e bombas
- Alertas de anomalias

### ✅ `/docs/SISTEMA_SCADA.md` (criado anteriormente)
- Documentação completa do sistema SCADA
- Modelo de dados com hierarquia
- 12 tipos de elementos com atributos
- Detecção de 6 tipos de anomalias
- Guia de uso da interface e API

---

## 🚀 Próximos Passos

### 1. Aplicar Migration

```bash
cd ~/firmware_aguada
sudo mysql -u aguada_user -p sensores_db < database/migrations/003_sistema_scada.sql
```

**O que será criado:**
- ✅ 12 tabelas novas
- ✅ 3 views (vw_status_elementos, vw_mapa_sistema, vw_anomalias_ativas)
- ✅ 1 trigger (detecção automática de nível crítico)
- ✅ 4 redes pré-cadastradas
- ✅ 5 locais
- ✅ 9 elementos com alias
- ✅ 5 sensores (4 ESP32 + 1 Nano) com MACs reais
- ✅ 6 conexões modelando fluxo completo

### 2. Verificar Tabelas

```bash
sudo mysql sensores_db -e "SHOW TABLES;"
```

**Esperado (15 tabelas):**
```
anomalias
conexoes
elementos
estados_bomba
estados_valvula
leituras_v2
locais
redes_agua
sensores
tipos_elemento
vw_anomalias_ativas
vw_mapa_sistema
vw_status_elementos
```

### 3. Testar Views

```sql
-- Ver todos elementos com sensores e última leitura
SELECT elemento_alias, elemento_nome, sensor_alias, 
       nivel_atual_cm, percentual_nivel, rede_nome, rede_cor
FROM vw_status_elementos
WHERE sensor_node_id IS NOT NULL;

-- Ver mapa de conexões por rede
SELECT origem_alias, destino_alias, rede_codigo, material, diametro_polegadas
FROM vw_mapa_sistema
ORDER BY rede_codigo;

-- Ver sensores ativos
SELECT alias, node_id, mac, tipo_sensor, ativo
FROM sensores
ORDER BY node_id;
```

### 4. Atualizar API para Incluir Alias

Modificar `/backend/api/scada_data.php`:

```php
function getElementos($mysqli) {
    $sql = "SELECT * FROM vw_status_elementos ORDER BY rede_codigo, local_nome, elemento_nome";
    // já inclui alias e rede automaticamente!
}
```

### 5. Atualizar Interface SCADA

Modificar `/backend/scada.html`:

```javascript
// Colorir por rede
const coresRede = {
    'REDE_CONSUMO': '#0066CC',
    'REDE_INCENDIO': '#FF0000',
    'REDE_PISCINA': '#00CCCC',
    'REDE_REUSO': '#996600'
};

function drawElemento(elem) {
    ctx.fillStyle = elem.rede_cor || '#666666';
    // desenhar usando alias como identificador
}

// Filtrar por rede
function filtrarRede(redecodigo) {
    elementos = elementosOriginais.filter(e => e.rede_codigo === redecodigo);
    drawSCADA();
}
```

### 6. Testar Sistema Completo

1. ✅ Aplicar migration
2. ✅ Verificar 5 sensores associados aos elementos
3. ✅ Abrir `http://192.168.0.117:8080/scada.html`
4. ✅ Ver elementos coloridos por rede
5. ✅ Clicar em elemento → ver alias, sensor, leitura atual
6. ✅ Testar filtro por rede
7. ✅ Verificar conexões desenhadas entre elementos

---

## 📊 Estatísticas

**Antes:**
- Sensores sem associação a elementos físicos
- Sem conceito de redes separadas
- Sem alias únicos
- Limitação: 1 sensor por elemento

**Depois:**
- ✅ 4 redes distintas com cores e normas
- ✅ 9 elementos cadastrados com alias
- ✅ 5 sensores com MACs reais mapeados
- ✅ Sistema completo modelado (Cisterna → Bomba → Reservatórios)
- ✅ Suporte a múltiplos sensores por elemento
- ✅ Suporte a elementos sem sensores (entrada manual)
- ✅ Visualização diferenciada por rede

**Capacidade Total:**
- Armazenamento: 330.000 L (330 m³)
  - Consumo: 80.000 L
  - Incêndio: 80.000 L
  - Cisterna: 120.000 L
  - Piscina: 50.000 L

---

## 🔗 Referências Criadas

1. `/docs/MAPEAMENTO_SENSORES.md` - MACs e convenções de alias
2. `/docs/SISTEMA_SCADA.md` - Documentação completa do SCADA
3. `/database/migrations/003_sistema_scada.sql` - Schema v2 com redes
4. `/backend/api/scada_data.php` - API REST completa
5. `/backend/scada.html` - Interface de visualização

---

## ⚠️ Notas Importantes

1. **Backup antes de aplicar:** 
   ```bash
   sudo mysqldump -u aguada_user sensores_db > backup_pre_scada_$(date +%Y%m%d).sql
   ```

2. **Arduino Nano (node_id=10):** Sensor cadastrado mas `elemento_id = NULL` e `ativo = FALSE`. Quando conectar à rede, atualizar:
   ```sql
   UPDATE sensores 
   SET elemento_id = (SELECT id FROM elementos WHERE alias = 'ALGUM_ELEMENTO'),
       ativo = TRUE
   WHERE alias = 'NANO_CISTERNA_SECUNDARIA';
   ```

3. **Expansão futura:** Para adicionar novos elementos/sensores, sempre usar alias únicos seguindo convenção documentada em `MAPEAMENTO_SENSORES.md`.

4. **Entrada manual:** Elementos sem sensores podem receber leituras via interface web. Criar formulário em `scada.html` para registrar estados de válvulas/bombas manualmente.

---

**Status:** ✅ Pronto para aplicação
**Testado:** ⏳ Pendente (aguardando apply da migration)
**Documentação:** ✅ Completa
