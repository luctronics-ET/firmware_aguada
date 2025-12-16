
import React, { useState, useEffect } from 'react';
import { ReservatorioAguada } from '@/entities/ReservatorioAguada';
import { LocalAguada } from '@/entities/LocalAguada';
import { Button } from "@/components/ui/button";
import { Dialog, DialogContent, DialogHeader, DialogTitle, DialogTrigger } from "@/components/ui/dialog";
import { Input } from "@/components/ui/input";
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select";
import { PlusCircle, Edit, Trash2 } from 'lucide-react';

const ReservatorioForm = ({ reservatorio, onSave, onCancel }) => {
  const [formData, setFormData] = useState(reservatorio || {
    nome: '', tipo: 'consumo', local_ref: '', capacidade_total: '',
    area_base: '', altura: '', sensor_id: '', estado: 'OP',
    volume_atual: 0, percentual_nivel: 0, status_alarme: 'normal'
  });
  const [locais, setLocais] = useState([]);

  useEffect(() => {
    const loadLocais = async () => {
      const data = await LocalAguada.list('nome');
      setLocais(data);
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

  const handleSubmit = (e) => {
    e.preventDefault();
    onSave(formData);
  };

  return (
    <form onSubmit={handleSubmit} className="space-y-4">
      <Input name="nome" value={formData.nome} onChange={handleChange} placeholder="Nome do Reservatório" required />
      
      <div className="grid grid-cols-2 gap-4">
        <Select name="tipo" value={formData.tipo} onValueChange={(v) => handleSelectChange('tipo', v)}>
          <SelectTrigger><SelectValue /></SelectTrigger>
          <SelectContent>
            <SelectItem value="consumo">Consumo</SelectItem>
            <SelectItem value="incendio">Incêndio</SelectItem>
            <SelectItem value="salgada">Salgada</SelectItem>
          </SelectContent>
        </Select>
        
        <Select name="local_ref" value={formData.local_ref} onValueChange={(v) => handleSelectChange('local_ref', v)} required>
          <SelectTrigger><SelectValue placeholder="Selecionar Local" /></SelectTrigger>
          <SelectContent>
            {locais.map(local => (
              <SelectItem key={local.id} value={local.codigo}>{local.nome}</SelectItem>
            ))}
          </SelectContent>
        </Select>
      </div>

      <div className="grid grid-cols-3 gap-4">
        <Input name="capacidade_total" type="number" step="0.1" value={formData.capacidade_total} onChange={handleChange} placeholder="Capacidade (T)" required />
        <Input name="area_base" type="number" step="0.01" value={formData.area_base} onChange={handleChange} placeholder="Área Base (m²)" />
        <Input name="altura" type="number" step="0.01" value={formData.altura} onChange={handleChange} placeholder="Altura (m)" />
      </div>

      <div className="grid grid-cols-2 gap-4">
        <Input name="sensor_id" value={formData.sensor_id} onChange={handleChange} placeholder="ID do Sensor" required />
        <Select name="estado" value={formData.estado} onValueChange={(v) => handleSelectChange('estado', v)}>
          <SelectTrigger><SelectValue /></SelectTrigger>
          <SelectContent>
            <SelectItem value="OP">OP - Operacional</SelectItem>
            <SelectItem value="INOP">INOP - Inoperante</SelectItem>
            <SelectItem value="MP">MP - Manutenção Preventiva</SelectItem>
            <SelectItem value="MC">MC - Manutenção Corretiva</SelectItem>
          </SelectContent>
        </Select>
      </div>
      
      <div className="flex justify-end gap-2">
        <Button type="button" variant="outline" onClick={onCancel}>Cancelar</Button>
        <Button type="submit">Salvar</Button>
      </div>
    </form>
  );
};

export default function ReservatoriosConfig() {
  const [reservatorios, setReservatorios] = useState([]);
  const [locais, setLocais] = useState([]);
  const [isDialogOpen, setIsDialogOpen] = useState(false);
  const [editingItem, setEditingItem] = useState(null);

  useEffect(() => { loadData() }, []);

  const loadData = async () => {
    const [reservatoriosData, locaisData] = await Promise.all([
      ReservatorioAguada.list(),
      LocalAguada.list('nome')
    ]);
    const unicos = reservatoriosData.filter((r, i, self) => i === self.findIndex(t => t.nome === r.nome && t.tipo === r.tipo));
    setReservatorios(unicos);
    setLocais(locaisData);
  };

  const handleSave = async (data) => {
    const payload = { ...data, ultima_leitura: new Date().toISOString() };
    if (editingItem) {
      await ReservatorioAguada.update(editingItem.id, payload);
    } else {
      await ReservatorioAguada.create(payload);
    }
    setEditingItem(null);
    setIsDialogOpen(false);
    loadData();
  };

  const handleDelete = async (id) => {
    if (window.confirm("Tem certeza que deseja remover este reservatório?")) {
      await ReservatorioAguada.delete(id);
      loadData();
    }
  };

  const getLocalNome = (localRef) => {
    const local = locais.find(l => l.codigo === localRef);
    return local ? local.nome : localRef || 'N/A';
  };

  const getEstadoColor = (estado) => {
    const colors = {
      OP: 'text-green-400',
      INOP: 'text-red-400',
      MP: 'text-amber-400',
      MC: 'text-orange-400'
    };
    return colors[estado] || 'text-slate-400';
  };

  return (
    <div>
      <Dialog open={isDialogOpen} onOpenChange={setIsDialogOpen}>
        <DialogTrigger asChild>
          <Button onClick={() => setEditingItem(null)}>
            <PlusCircle className="w-4 h-4 mr-2" /> Adicionar Reservatório
          </Button>
        </DialogTrigger>
        <DialogContent className="max-w-2xl">
          <DialogHeader>
            <DialogTitle>{editingItem ? 'Editar' : 'Adicionar'} Reservatório</DialogTitle>
          </DialogHeader>
          <ReservatorioForm reservatorio={editingItem} onSave={handleSave} onCancel={() => setIsDialogOpen(false)} />
        </DialogContent>
      </Dialog>
      
      <div className="mt-4 bg-slate-800/50 border border-slate-700/80 rounded-lg overflow-hidden">
        <table className="min-w-full text-left text-sm">
          <thead className="bg-slate-800">
            <tr>
              <th className="px-4 py-2">Nome</th>
              <th className="px-4 py-2">Tipo</th>
              <th className="px-4 py-2">Local</th>
              <th className="px-4 py-2">Capacidade</th>
              <th className="px-4 py-2">Dimensões</th>
              <th className="px-4 py-2">Sensor</th>
              <th className="px-4 py-2">Estado</th>
              <th className="px-4 py-2">Ações</th>
            </tr>
          </thead>
          <tbody>
            {reservatorios.map(item => (
              <tr key={item.id} className="border-t border-slate-800 hover:bg-slate-800/40">
                <td className="px-4 py-2 font-medium text-slate-200">{item.nome}</td>
                <td className="px-4 py-2 capitalize">
                  <span className="px-2 py-1 text-xs bg-slate-700/50 rounded border border-slate-600">
                    {item.tipo}
                  </span>
                </td>
                <td className="px-4 py-2">
                  <span className="text-xs bg-slate-700/50 px-2 py-1 rounded border border-slate-600">
                    {getLocalNome(item.local_ref)}
                  </span>
                </td>
                <td className="px-4 py-2">{item.capacidade_total} T</td>
                <td className="px-4 py-2 text-xs text-slate-400">
                  {item.area_base && item.altura ? `${item.area_base}m² × ${item.altura}m` : 'N/A'}
                </td>
                <td className="px-4 py-2 text-xs font-mono text-slate-300">{item.sensor_id}</td>
                <td className="px-4 py-2">
                  <span className={`px-2 py-1 text-xs font-medium rounded bg-slate-600/20 ${getEstadoColor(item.estado)}`}>
                    {item.estado}
                  </span>
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

