# Mapeamento de Sensores - Projeto Aguada

## Sensores Ultrassônicos Instalados

### Dispositivos ESP32 (ESP-NOW)

| Node ID | MAC Address         | Alias    | Elemento                      | Local                  | Status |
|---------|-------------|-------|----------|------                        -|--------|
| 1       | `20:6E:F1:6B:77:58` | `RCON` | Reservatório Castelo Consumo    | Castelo de Consumo     | ✅ Ativo |
| 2       | `DC:06:75:67:6A:CC` | `RCAV` | Reservatório Castelo Incêndio   | Castelo de Incêndio    | ✅ Ativo |
| 3       | `80:F1:B2:50:31:34` | `RCB3` | Reservatório Casa Bombas N03    | Casa Bombas N03        | ✅ Ativo |
| 4       | `DC:B4:D9:8B:9E:AC` | `CIE1` | Cisterna Ilha Engenho N01       | Cisternas Ilha Engenho | ✅ Ativo |
| 5       | `AA:BB:CC:DD:IE:02` | `CIE2` | Cisterna Ilha Engenho N02       | Cisternas Ilha Engenho | ✅ Ativo |

### Dispositivos Arduino Nano (Ethernet)

| Node ID | MAC Address    | Alias  | Elemento                        | Local | Status |
|---------|------------   -|------- |-------                       ---|-------|--------|
| 10 | `DE:AD:BE:EF:FE:ED` | `RCAV2`| Reservatório Castelo Incêndio   | Castelo de Incêndio    | ✅ Ativo |
---

## Convenção de Nomes valvulas e encanamentos (Alias)

### Formato 
```
VALV_RCON_IN1-<-RCB3_OUT          (VALVULA DE ENTRADA PARA RESERVATORIO)
VALV_RCON_OUT1->-AZCON01_IN1      (VALVULA DE SAIDA DO RESERVATORIO)


VALV_RCAV_IN1-<-RCB3_OUT1
VALV_RCAV_OUT1->-AZCAV1_IN1              ([ELEMENTO]_[PT-INICIO]->[PT-FIM])
VALV_RCAV_OUT2->-AZCAV2_IN1
VALV_RCAV_OUT3->-AVCAV1_IN1
VALV_AZCAV0_OUT1->-AVCAV0_IN1

VALVY_RCB3_OUT1==>RCON_IN1==>RCAV_IN1   ([ELEMENTO]_[PT-INICIO]==>[PT-FIM1]==>[PT_FIM2])
(ENTAON TEMOS SAIDA DE RCB3-OUT1 -- VALVY) -----------------------RCON_IN1```
   
                                    VALVY-------------------------RCAV_IN01    -------- SERIA ENCANAMENTO (MAIS LONGOS OU PRINCIPAIS SERAO MAPEADOS)

[NAO INVENTE NOMES, TENTE CRIAR LIGACOES OBVIAS, MAS PERGUNTE ANTES PARA EVIRAR RETRABALHO.]

EM Casa Bombas N03  HA UMA BOMBA RECALQUE DIESEL E UMA ELETRICA. BOR-CB3-MD1   (BOMBA RECALQUE DA CB3 - MOTOR DIESEL 01) E (BOR-CB3-ME01) -MOTOR ELETRICO

BASICAMENTE, A AGUA VEM DA CONCESSIONARIA PARA A CISTERNA IF1 E IF2, PASSA NA CASA BOMBAS IF, VAI PARA RESERVATORIO CASA BOMBAS ILHA DAS FLORES RCIF (80M3), PASSA PELAS BOMBAS E VAI PARA CISTERNAS IE1 E IE2. DAS CISTERNAS IE1 E IE2 PASSA NA CASA BOMBAS N03, RCB03, BOMBAS CB03, RCON E/OU (DEPENDENDO DE VALVULAS) RCAV. DE RCON E RCAV VAO PARA REDES DE HIDRANTES (E AGUA E CONSUMIDA) OU REDES DE EDIFICIOS (E AGUA CONSUMO E CONSUMIDA)


### Prefixos por Tipo
- `BOR_` - Bombas (recalque
,- `BOP_` - Bombas pressão)
- `VALV_` - Válvulas
- `VALVY_` - Válvulas Y
- `HIDRO_` - Hidrômetros
- `RES_` - Reservatórios ELEVADOS
- `CIS_` - Cisternas SUBTERRANEAS
- `HIDY_` - Hidrantes Y
- `ENC_` - Encanamentos principais

---

## Elementos com Múltiplos Sensores

Alguns elementos podem ter **mais de um sensor** para redundância ou medições diferentes:

### Exemplo 1: Cisterna com 2 Sensores
```
Cisterna Principal (120.000 L)
  ├── ULTRA_CISTERNA_TOPO (node_id=3) - Medição pelo topo
  └── ULTRA_CISTERNA_LATERAL (node_id=11) - Medição lateral (redundância)
```

**Configuração:**
- Sensores medem o mesmo elemento mas de posições diferentes
- Sistema calcula **média** ou detecta **discrepância** (alerta vazamento/falha)

### Exemplo 2: Reservatório com Sensor + Hidrômetro
```
Reservatório Consumo (80.000 L)
  ├── ULTRA_CASTELO_CONSUMO (node_id=1) - Nível interno
  └── HIDRO_ENTRADA_CONSUMO (node_id=20) - Volume entrando
```

**Uso:**
- Comparar volume medido por nível vs volume acumulado no hidrômetro
- Detectar vazamentos: `Δvolume_sensor ≠ Δvolume_hidrômetro`

---

## Elementos Sem Sensores (Entrada Manual)

Nem todos elementos precisam de sensores automáticos. Alguns têm **entrada manual** via interface web:

### Válvulas Manuais
```
Válvula Entrada Incêndio (VALV_ENTRADA_INCENDIO)
  └── Operação manual registrada via interface SCADA
```

### Bombas com Leitura Manual
```
Bomba Pressão 02 (BOMBA_PRESSAO_02)
  └── Operador registra: Pressão (manômetro), Corrente (amperímetro)
```

### Hidrômetros Analógicos
```
Hidrômetro Entrada Geral (HIDRO_ENTRADA_GERAL)
  └── Leitura mensal manual do contador mecânico
```

**Interface Web:** Formulário em `backend/scada.html` para:
- Registrar leitura de hidrômetro (m³)
- Atualizar estado de válvula (aberta/fechada/parcial %)
- Registrar status de bomba (ligada/desligada, pressão, corrente)

---

## Redes de Água (Tipos/Finalidades)

O sistema hidráulico é dividido em **redes independentes** por qualidade e finalidade:

### 1. Rede de Consumo (Água Potável)
**Finalidade:** Abastecimento residencial (torneiras, chuveiros, lavatórios)

**Componentes:**
```
Cisterna Principal (CIS_PRINCIPAL)
  └─> Bomba Recalque 01 (BOMBA_RECALQUE_01)
      └─> Válvula Geral Consumo (VALV_GERAL_CONSUMO)
          └─> Reservatório Castelo Consumo (RES_CASTELO_CONSUMO) [80.000 L]
              └─> Hidrômetro Saída (HIDRO_SAIDA_CONSUMO)
                  └─> Rede Distribuição Predial
```

**Sensores:**
- `ULTRA_CISTERNA_PRINCIPAL` (node_id=3)
- `ULTRA_CASTELO_CONSUMO` (node_id=1)
- `HIDRO_SAIDA_CONSUMO` (manual)

**Qualidade:** Água tratada, potável, clorada

---

### 2. Rede de Incêndio
**Finalidade:** Combate a incêndio (hidrantes Y para conexão de mangueiras)

**Componentes:**
```
Reservatório Incêndio (RES_CASTELO_INCENDIO) [80.000 L]
  ├─> Entrada: Válvula Geral Entrada Incêndio (VALV_ENTRADA_INCENDIO)
  │   └─> Encanamento 4" ferro galvanizado (TUBO_ENTRADA_INCENDIO)
  │       └─> Casa de Bombas N03 (LOCAL_CASA_BOMBAS_03)
  │           └─> Válvula Y Distribuição (VALV_Y_DISTRIBUICAO_INCENDIO)
  │
  └─> Saída: Rede de Hidrantes
      ├─> Hidrante Y Térreo (HIDR_Y_TERREO)
      ├─> Hidrante Y 1º Andar (HIDR_Y_1ANDAR)
      └─> Hidrante Y 2º Andar (HIDR_Y_2ANDAR)
```

**Sensores:**
- `ULTRA_CASTELO_INCENDIO` (node_id=2)
- Pressão na rede (manômetros manuais)
- Hidrantes sem sensor (verificação visual mensal)

**Qualidade:** Água não potável, sem tratamento especial, pressão mínima 40 mH₂O (4 bar)

**Normas:** NBR 13714 (Hidrantes), NR-23 (Proteção contra Incêndios)

---

### 3. Rede de Piscina (Recirculação)
**Finalidade:** Piscina + tratamento (filtro, cloração, aquecimento)

**Componentes:**
```
Reservatório Piscina (RES_PISCINA) [50.000 L]
  └─> Bomba Recirculação (BOMBA_RECIRC_PISCINA)
      └─> Filtro Areia (FILTRO_AREIA_PISCINA)
          └─> Aquecedor Solar (AQUEC_SOLAR_PISCINA)
              └─> Retorno Piscina (válvulas direcionais)
```

**Sensores:**
- `ULTRA_RESERVATORIO_PISCINA` (node_id=4)
- Sensor pH (futuro)
- Sensor ORP/cloro (futuro)
- Sensor temperatura (futuro)

**Qualidade:** Água tratada mas não potável, pH 7.2-7.6, cloro 1-3 ppm

---

### 4. Rede de Reuso (Água Cinza)
**Finalidade:** Descarga sanitária, irrigação jardim, lavagem pisos

**Componentes:**
```
Cisterna Reuso (CIS_REUSO) [30.000 L]
  ├─> Entrada: Água de Chuva (calhas)
  │           Água Cinza (lavatórios, chuveiros)
  │
  └─> Saída: Bomba Pressão Reuso (BOMBA_PRESSAO_REUSO)
      ├─> Vasos Sanitários
      └─> Sistema Irrigação Jardim
```

**Sensores:**
- `NANO_CISTERNA_SECUNDARIA` (node_id=10) - pendente rede
- Hidrômetro entrada chuva (manual)

**Qualidade:** Água não potável, filtração básica, uso restrito

---

## Modelo de Rede no Banco de Dados

### Tabela `redes_agua`

```sql
CREATE TABLE redes_agua (
    id INT PRIMARY KEY AUTO_INCREMENT,
    codigo VARCHAR(50) UNIQUE NOT NULL,  -- 'REDE_CONSUMO', 'REDE_INCENDIO'
    nome VARCHAR(100) NOT NULL,
    finalidade TEXT,
    qualidade_agua ENUM('potavel', 'nao_potavel', 'industrial', 'reuso') DEFAULT 'nao_potavel',
    pressao_minima_bar DECIMAL(5,2),
    pressao_maxima_bar DECIMAL(5,2),
    cor_diagrama VARCHAR(7),  -- '#0066CC' para consumo, '#FF0000' para incêndio
    norma_aplicavel VARCHAR(100),  -- 'NBR 13714', 'NBR 5626'
    ativo BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

### Associar Elementos à Rede

```sql
ALTER TABLE elementos 
ADD COLUMN rede_agua_id INT,
ADD FOREIGN KEY (rede_agua_id) REFERENCES redes_agua(id);
```

**Exemplo:**
```sql
UPDATE elementos SET rede_agua_id = 1 WHERE alias IN ('RES_CASTELO_CONSUMO', 'BOMBA_RECALQUE_01');
UPDATE elementos SET rede_agua_id = 2 WHERE alias IN ('RES_CASTELO_INCENDIO', 'HIDR_Y_TERREO');
```

---

## Visualização no SCADA

### Diagrama com Cores por Rede

```javascript
const coresRede = {
    'REDE_CONSUMO': '#0066CC',      // Azul
    'REDE_INCENDIO': '#FF0000',     // Vermelho
    'REDE_PISCINA': '#00CCCC',      // Ciano
    'REDE_REUSO': '#996600'         // Marrom
};

function drawElemento(elem) {
    ctx.fillStyle = coresRede[elem.rede_codigo] || '#666666';
    // desenhar círculo, ícone, etc
}
```

### Filtros por Rede

```html
<div class="filtros">
    <button onclick="filtrarRede('REDE_CONSUMO')">🚰 Consumo</button>
    <button onclick="filtrarRede('REDE_INCENDIO')">🔥 Incêndio</button>
    <button onclick="filtrarRede('REDE_PISCINA')">🏊 Piscina</button>
    <button onclick="filtrarRede('REDE_REUSO')">♻️ Reuso</button>
</div>
```

---

## Checklist de Implementação

- [ ] Atualizar migration `003_sistema_scada.sql`:
  - [ ] Adicionar campo `alias` em `elementos` (UNIQUE, NOT NULL)
  - [ ] Criar tabela `redes_agua`
  - [ ] Adicionar FK `rede_agua_id` em `elementos`
  - [ ] Permitir múltiplos sensores por elemento (remover UNIQUE de `sensores.elemento_id`)
  - [ ] Inserir dados de exemplo com alias

- [ ] Atualizar `backend/api/scada_data.php`:
  - [ ] Incluir `alias` nos elementos retornados
  - [ ] Incluir `rede_agua` com cor/nome
  - [ ] Endpoint para entrada manual de leituras

- [ ] Atualizar `backend/scada.html`:
  - [ ] Colorir elementos por rede
  - [ ] Filtro por rede
  - [ ] Modal para entrada manual (válvulas, bombas, hidrômetros)
  - [ ] Exibir múltiplos sensores por elemento

- [ ] Documentar MACs reais dos 5 sensores instalados ✅

---

## Referências

- **NBR 5626**: Instalação predial de água fria
- **NBR 13714**: Sistemas de hidrantes e mangotinhos
- **NBR 15527**: Água de chuva - Aproveitamento
- **NR-23**: Proteção contra incêndios
- **Portaria MS 2914/2011**: Padrões de potabilidade
