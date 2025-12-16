# 📁 .REFERENCIAS - Diretório de Referências Externas

## ⚠️ ATENÇÃO - AI/Copilot Instructions

**Esta pasta NÃO faz parte do sistema principal do projeto `firmware_aguada`.**

### 🎯 Propósito

Este diretório é usado exclusivamente para:

1. **Armazenamento temporário** de arquivos baixados via `fetch_webpage`
2. **Templates e referências** externas para consulta
3. **Snippets de código** copiados de documentações externas
4. **Exemplos e demos** de terceiros (ex: TailAdmin, bibliotecas JS/CSS)

### 🚫 Regras para AI/Copilot

**NUNCA:**
- ❌ Inclua arquivos desta pasta em análises de código do projeto
- ❌ Sugira modificações em arquivos desta pasta
- ❌ Use código desta pasta diretamente sem adaptação
- ❌ Considere esta pasta como parte da estrutura do projeto
- ❌ Faça commit de arquivos desta pasta no Git

**SEMPRE:**
- ✅ Ignore esta pasta ao buscar código do projeto (`grep_search`, `semantic_search`)
- ✅ Use arquivos desta pasta APENAS como **referência visual/conceitual**
- ✅ Adapte e reescreva código antes de usar no projeto real
- ✅ Mantenha esta pasta em `.gitignore`

### 📂 Estrutura Típica

```
.REFERENCIAS/
├── tailadmin-templates/    # Templates de UI baixados
├── library-docs/          # Documentações baixadas
├── code-snippets/         # Trechos de código de exemplo
└── design-inspirations/   # Mockups e designs de referência
```

### 🔍 Quando Usar

**Use esta pasta quando o usuário pedir:**
- "Busque exemplos de [tecnologia X]"
- "Baixe o template [nome]"
- "Salve essa documentação para referência"
- "Fetch [URL] para consulta"

**Arquivos aqui são apenas INSPIRAÇÃO, não código de produção.**

### 🛠️ Workflow Recomendado

1. **Fetch/Download** → Salvar em `.REFERENCIAS/`
2. **Análise** → Estudar estrutura e conceitos
3. **Adaptação** → Reescrever código para o projeto
4. **Implementação** → Salvar no diretório correto do projeto
5. **Limpeza** → Opcionalmente remover arquivo de `.REFERENCIAS/`

### 📝 Exemplo de Uso

```bash
# ✅ CORRETO: Fetch para referência
fetch_webpage("https://exemplo.com/template") 
→ Salva em .REFERENCIAS/exemplo-template.html
→ Analisa estrutura
→ Recria em backend/nova_pagina.html (adaptado)

# ❌ ERRADO: Copiar diretamente
cp .REFERENCIAS/template.html backend/pagina.html  # NÃO FAZER!
```

---

**🤖 Esta instrução foi criada para guiar sistemas de IA na correta interpretação desta pasta.**