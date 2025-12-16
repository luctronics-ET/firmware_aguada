
import React, { useState, useEffect } from 'react';
import { SensorAguada } from '@/entities/SensorAguada';
import { LocalAguada } from '@/entities/LocalAguada';
import { Button } from "@/components/ui/button";
import { Dialog, DialogContent, DialogHeader, DialogTitle, DialogTrigger } from "@/components/ui/dialog";
import { Input } from "@/components/ui/input";
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select";
import { PlusCircle, Edit, Trash2, Plus, X, Cpu, Activity } from 'lucide-react';

const EntradaConfig = ({ entrada, onChange, onRemove }) => (
  <div className="flex gap-2 items-end p-3 bg-slate-700/30 rounded border border-slate-600">
    <div className="flex-1">
      <label className="text-xs text-slate-400">Nome</label>
      <Input value={entrada.nome} onChange={(e) => onChange({...entrada, nome: e.target.value})} placeholder="Ex: Nível Castelo" />
    </div>
    <div className="w-32">
      <label className="text-xs text-slate-400">Tipo</label>
      <Select value={entrada.tipo} onValueChange={(v) => onChange({...entrada, tipo: v})}>
        <SelectTrigger><SelectValue /></SelectTrigger>
        <SelectContent>
          <SelectItem value="analogico">Analógico</SelectItem>
          <SelectItem value="digital">Digital</SelectItem>
          <SelectItem value="temperatura">Temperatura</SelectItem>
          <SelectItem value="pressao">Pressão</SelectItem>
          <SelectItem value="nivel">Nível</SelectItem>
        </SelectContent>
      </Select>
    </div>
    <div className="w-20">
      <label className="text-xs text-slate-400">Pin</label>
      <Input type="number" value={entrada.pin} onChange={(e) => onChange({...entrada, pin: parseInt(e.target.value)})} />
    </div>
    <div className="w-20">
      <label className="text-xs text-slate-400">Unidade</label>
      <Input value={entrada.unidade} onChange={(e) => onChange({...entrada, unidade: e.target.value})} placeholder="V, T, %" />
    </div>
    <Button variant="outline" size="sm" onClick={onRemove}>
      <X className="w-4 h-4" />
    </Button>
  </div>
);

const SaidaConfig = ({ saida, onChange, onRemove }) => (
  <div className="flex gap-2 items-end p-3 bg-slate-700/30 rounded border border-slate-600">
    <div className="flex-1">
      <label className="text-xs text-slate-400">Nome</label>
      <Input value={saida.nome} onChange={(e) => onChange({...saida, nome: e.target.value})} placeholder="Ex: Bomba 1" />
    </div>
    <div className="w-32">
      <label className="text-xs text-slate-400">Tipo</label>
      <Select value={saida.tipo} onValueChange={(v) => onChange({...saida, tipo: v})}>
        <SelectTrigger><SelectValue /></SelectTrigger>
        <SelectContent>
          <SelectItem value="digital">Digital</SelectItem>
          <SelectItem value="pwm">PWM</SelectItem>
          <SelectItem value="rele">Relé</SelectItem>
        </SelectContent>
      </Select>
    </div>
    <div className="w-20">
      <label className="text-xs text-slate-400">Pin</label>
      <Input type="number" value={saida.pin} onChange={(e) => onChange({...saida, pin: parseInt(e.target.value)})} />
    </div>
    <Button variant="outline" size="sm" onClick={onRemove}>
      <X className="w-4 h-4" />
    </Button>
  </div>
);

const SensorForm = ({ sensor, onSave, onCancel }) => {
  const [formData, setFormData] = useState(sensor || {
    mac_address: '', node_id: '', local_ref: '', entradas: [], saidas: [], 
    status: 'offline', versao_firmware: ''
  });
  const [locais, setLocais] = useState([]);

  useEffect(() => {
    const loadLocais = async () => {
      try {
        const data = await LocalAguada.list('nome'); // Changed to pass 'nome' for sorting
        setLocais(data);
      } catch (error) {
        console.error("Error loading locais:", error);
      }
    };
    loadLocais();
  }, []);

  const handleChange = (e) => {
    const { name, value } = e.target;
    setFormData(prev => ({ ...prev, [name]: value }));
  };

  const handleSelectChange = (name, value) => {
    setFormData(prev => ({ ...prev, [name]: value }));
  };

  const adicionarEntrada = () => {
    setFormData(prev => ({
      ...prev, 
      entradas: [...prev.entradas, { nome: '', tipo: 'analogico', unidade: '', valor: 0, pin: 0 }]
    }));
  };

  const adicionarSaida = () => {
    setFormData(prev => ({
      ...prev,
      saidas: [...prev.saidas, { nome: '', tipo: 'digital', pin: 0, estado: false }]
    }));
  };

  const updateEntrada = (index, entrada) => {
    const novasEntradas = [...formData.entradas];
    novasEntradas[index] = entrada;
    setFormData(prev => ({ ...prev, entradas: novasEntradas }));
  };

  const updateSaida = (index, saida) => {
    const novasSaidas = [...formData.saidas];
    novasSaidas[index] = saida;
    setFormData(prev => ({ ...prev, saidas: novasSaidas }));
  };

  const removerEntrada = (index) => {
    setFormData(prev => ({
      ...prev,
      entradas: prev.entradas.filter((_, i) => i !== index)
    }));
  };

  const removerSaida = (index) => {
    setFormData(prev => ({
      ...prev,
      saidas: prev.saidas.filter((_, i) => i !== index)
    }));
  };

  const handleSubmit = (e) => {
    e.preventDefault();
    onSave(formData);
  };

  return (
    <div className="max-h-[80vh] overflow-y-auto">
      <form onSubmit={handleSubmit} className="space-y-4">
        <div className="grid grid-cols-2 gap-4">
          <Input name="mac_address" value={formData.mac_address} onChange={handleChange} placeholder="MAC Address" required />
          <Input name="node_id" value={formData.node_id} onChange={handleChange} placeholder="Node ID" required />
        </div>
        <Select name="local_ref" value={formData.local_ref} onValueChange={(v) => handleSelectChange('local_ref', v)} required>
          <SelectTrigger><SelectValue placeholder="Selecionar Local" /></SelectTrigger>
          <SelectContent>
            {locais.map(local => (
              <SelectItem key={local.id} value={local.codigo}>{local.nome}</SelectItem>
            ))}
          </SelectContent>
        </Select>
        <div className="grid grid-cols-2 gap-4">
          <Select name="status" value={formData.status} onValueChange={(v) => handleSelectChange('status', v)}>
            <SelectTrigger><SelectValue /></SelectTrigger>
            <SelectContent>
              <SelectItem value="online">Online</SelectItem>
              <SelectItem value="offline">Offline</SelectItem>
              <SelectItem value="erro">Erro</SelectItem>
            </SelectContent>
          </Select>
          <Input name="versao_firmware" value={formData.versao_firmware} onChange={handleChange} placeholder="Versão Firmware" />
        </div>

        <div>
          <div className="flex items-center justify-between mb-2">
            <label className="text-sm font-medium text-slate-300">Entradas</label>
            <Button type="button" variant="outline" size="sm" onClick={adicionarEntrada}>
              <Plus className="w-4 h-4 mr-1" /> Adicionar
            </Button>
          </div>
          <div className="space-y-2 max-h-60 overflow-y-auto">
            {formData.entradas.map((entrada, index) => (
              <EntradaConfig 
                key={index} 
                entrada={entrada} 
                onChange={(e) => updateEntrada(index, e)}
                onRemove={() => removerEntrada(index)}
              />
            ))}
          </div>
        </div>

        <div>
          <div className="flex items-center justify-between mb-2">
            <label className="text-sm font-medium text-slate-300">Saídas</label>
            <Button type="button" variant="outline" size="sm" onClick={adicionarSaida}>
              <Plus className="w-4 h-4 mr-1" /> Adicionar
            </Button>
          </div>
          <div className="space-y-2 max-h-60 overflow-y-auto">
            {formData.saidas.map((saida, index) => (
              <SaidaConfig
                key={index}
                saida={saida}
                onChange={(s) => updateSaida(index, s)}
                onRemove={() => removerSaida(index)}
              />
            ))}
          </div>
        </div>

        <div className="flex justify-end gap-2">
          <Button type="button" variant="outline" onClick={onCancel}>Cancelar</Button>
          <Button type="submit">Salvar</Button>
        </div>
      </form>
    </div>
  );
};

export default function SensoresConfig() {
  const [sensores, setSensores] = useState([]);
  const [locais, setLocais] = useState([]); // Added new state for locals
  const [isDialogOpen, setIsDialogOpen] = useState(false);
  const [editingItem, setEditingItem] = useState(null);

  useEffect(() => { loadData() }, []);

  const loadData = async () => {
    // Fetch both sensors and locals concurrently
    const [sensoresData, locaisData] = await Promise.all([
      SensorAguada.list(),
      LocalAguada.list('nome') // Load locals, sorted by 'nome' for consistency with SensorForm
    ]);
    setSensores(sensoresData);
    setLocais(locaisData); // Set the loaded locals
  };

  const getLocalNome = (localRef) => {
    const local = locais.find(l => l.codigo === localRef);
    return local ? local.nome : localRef || 'N/A';
  };

  const handleSave = async (data) => {
    // Ensure local_ref is not an empty string if not selected
    const payload = { 
      ...data, 
      ultima_comunicacao: new Date().toISOString(),
      local_ref: data.local_ref === '' ? null : data.local_ref // Save as null if empty
    };
    if (editingItem) {
      await SensorAguada.update(editingItem.id, payload);
    } else {
      await SensorAguada.create(payload);
    }
    setEditingItem(null);
    setIsDialogOpen(false);
    loadData();
  };

  const handleDelete = async (id) => {
    if (window.confirm("Tem certeza que deseja remover este sensor?")) {
      await SensorAguada.delete(id);
      loadData();
    }
  };

  const getStatusColor = (status) => {
    const colors = {
      online: 'text-green-400',
      offline: 'text-slate-400', 
      erro: 'text-red-400'
    };
    return colors[status] || 'text-slate-400';
  };

  return (
    <div>
      <Dialog open={isDialogOpen} onOpenChange={setIsDialogOpen}>
        <DialogTrigger asChild>
          <Button onClick={() => setEditingItem(null)}>
            <PlusCircle className="w-4 h-4 mr-2" /> Adicionar Sensor
          </Button>
        </DialogTrigger>
        <DialogContent className="max-w-4xl">
          <DialogHeader>
            <DialogTitle>{editingItem ? 'Editar' : 'Adicionar'} Sensor ESP32</DialogTitle>
          </DialogHeader>
          <SensorForm sensor={editingItem} onSave={handleSave} onCancel={() => setIsDialogOpen(false)} />
        </DialogContent>
      </Dialog>
      
      <div className="mt-4 bg-slate-800/50 border border-slate-700/80 rounded-lg overflow-hidden">
        <table className="min-w-full text-left text-sm">
          <thead className="bg-slate-800">
            <tr>
              <th className="px-4 py-2">Node ID</th>
              <th className="px-4 py-2">Local</th>
              <th className="px-4 py-2">Status</th>
              <th className="px-4 py-2">MAC Address</th>
              <th className="px-4 py-2">Entradas/Saídas</th>
              <th className="px-4 py-2">Ações</th>
            </tr>
          </thead>
          <tbody>
            {sensores.map(item => (
              <tr key={item.id} className="border-t border-slate-800">
                <td className="px-4 py-2">
                  <div className="flex items-center gap-2">
                    <Cpu className="w-4 h-4 text-sky-400" />
                    <span className="font-medium">{item.node_id}</span>
                  </div>
                </td>
                <td className="px-4 py-2">
                  <span className="text-xs bg-slate-700/50 px-2 py-1 rounded border border-slate-600">
                    {getLocalNome(item.local_ref)} {/* Updated to display local name */}
                  </span>
                </td>
                <td className="px-4 py-2">
                  <div className="flex items-center gap-2">
                    <Activity className={`w-3 h-3 ${getStatusColor(item.status)}`} />
                    <span className={`text-xs font-medium capitalize ${getStatusColor(item.status)}`}>
                      {item.status}
                    </span>
                  </div>
                </td>
                <td className="px-4 py-2 font-mono text-xs">{item.mac_address}</td>
                <td className="px-4 py-2">
                  <div className="text-xs text-slate-400">
                    {item.entradas?.length || 0} entradas • {item.saidas?.length || 0} saídas
                  </div>
                </td>
                <td className="px-4 py-2">
                  <Button variant="ghost" size="sm" onClick={() => { setEditingItem(item); setIsDialogOpen(true); }}>
                    <Edit className="w-4 h-4" />
                  </Button>
                  <Button variant="ghost" size="sm" onClick={() => handleDelete(item.id)}>
                    <Trash2 className="w-4 h-4" />
                  </Button>
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  );
}

