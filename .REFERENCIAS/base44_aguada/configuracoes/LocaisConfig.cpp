import React, { useState, useEffect } from 'react';
import { LocalAguada } from '@/entities/LocalAguada';
import { Button } from "@/components/ui/button";
import { Dialog, DialogContent, DialogHeader, DialogTitle, DialogTrigger } from "@/components/ui/dialog";
import { Input } from "@/components/ui/input";
import { Textarea } from "@/components/ui/textarea";
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select";
import { PlusCircle, Edit, Trash2, MapPin, User } from 'lucide-react';

const LocalForm = ({ local, onSave, onCancel }) => {
  const [formData, setFormData] = useState(local || {
    nome: '', codigo: '', tipo: 'castelo', descricao: '', 
    coordenadas: { latitude: '', longitude: '' }, responsavel: 'OSE', observacoes: ''
  });

  const handleChange = (e) => {
    const { name, value } = e.target;
    setFormData(prev => ({ ...prev, [name]: value }));
  };

  const handleSelectChange = (name, value) => {
    setFormData(prev => ({ ...prev, [name]: value }));
  };

  const handleCoordChange = (campo, value) => {
    setFormData(prev => ({
      ...prev,
      coordenadas: { ...prev.coordenadas, [campo]: value ? parseFloat(value) : '' }
    }));
  };

  const handleSubmit = (e) => {
    e.preventDefault();
    onSave(formData);
  };

  return (
    <form onSubmit={handleSubmit} className="space-y-4">
      <div className="grid grid-cols-2 gap-4">
        <Input name="nome" value={formData.nome} onChange={handleChange} placeholder="Nome do Local" required />
        <Input name="codigo" value={formData.codigo} onChange={handleChange} placeholder="Código (ex: CST-01)" required />
      </div>
      
      <div className="grid grid-cols-2 gap-4">
        <Select name="tipo" value={formData.tipo} onValueChange={(v) => handleSelectChange('tipo', v)}>
          <SelectTrigger><SelectValue /></SelectTrigger>
          <SelectContent>
            <SelectItem value="castelo">Castelo</SelectItem>
            <SelectItem value="casa_bombas">Casa de Bombas</SelectItem>
            <SelectItem value="cisterna">Cisterna</SelectItem>
            <SelectItem value="rede_distribuicao">Rede de Distribuição</SelectItem>
            <SelectItem value="laboratorio">Laboratório</SelectItem>
          </SelectContent>
        </Select>
        
        <Select name="responsavel" value={formData.responsavel} onValueChange={(v) => handleSelectChange('responsavel', v)}>
          <SelectTrigger><SelectValue /></SelectTrigger>
          <SelectContent>
            <SelectItem value="OSE">OSE</SelectItem>
            <SelectItem value="CB_AUX">CB AUX</SelectItem>
            <SelectItem value="OF_CAV">OF CAV</SelectItem>
          </SelectContent>
        </Select>
      </div>

      <Textarea name="descricao" value={formData.descricao} onChange={handleChange} placeholder="Descrição do local..." />
      
      <div className="space-y-2">
        <label className="text-sm font-medium text-gray-700">Coordenadas GPS (Opcional)</label>
        <div className="grid grid-cols-2 gap-4">
          <Input 
            type="number" 
            step="any"
            value={formData.coordenadas.latitude} 
            onChange={(e) => handleCoordChange('latitude', e.target.value)} 
            placeholder="Latitude" 
          />
          <Input 
            type="number" 
            step="any"
            value={formData.coordenadas.longitude} 
            onChange={(e) => handleCoordChange('longitude', e.target.value)} 
            placeholder="Longitude" 
          />
        </div>
      </div>

      <Textarea name="observacoes" value={formData.observacoes} onChange={handleChange} placeholder="Observações..." />

      <div className="flex justify-end gap-2">
        <Button type="button" variant="outline" onClick={onCancel}>Cancelar</Button>
        <Button type="submit">Salvar</Button>
      </div>
    </form>
  );
};

export default function LocaisConfig() {
  const [locais, setLocais] = useState([]);
  const [isDialogOpen, setIsDialogOpen] = useState(false);
  const [editingItem, setEditingItem] = useState(null);

  useEffect(() => { loadData() }, []);

  const loadData = async () => {
    const data = await LocalAguada.list('nome');
    setLocais(data);
  };

  const handleSave = async (data) => {
    if (editingItem) {
      await LocalAguada.update(editingItem.id, data);
    } else {
      await LocalAguada.create(data);
    }
    setEditingItem(null);
    setIsDialogOpen(false);
    loadData();
  };

  const handleDelete = async (id) => {
    if (window.confirm("Tem certeza que deseja remover este local?")) {
      await LocalAguada.delete(id);
      loadData();
    }
  };

  const getTipoColor = (tipo) => {
    const colors = {
      castelo: 'text-sky-600',
      casa_bombas: 'text-orange-600',
      cisterna: 'text-cyan-600',
      rede_distribuicao: 'text-green-600',
      laboratorio: 'text-purple-600'
    };
    return colors[tipo] || 'text-gray-600';
  };

  const getResponsavelColor = (responsavel) => {
    const colors = {
      OSE: 'text-blue-600',
      CB_AUX: 'text-green-600',
      OF_CAV: 'text-amber-600'
    };
    return colors[responsavel] || 'text-gray-600';
  };

  return (
    <div>
      <Dialog open={isDialogOpen} onOpenChange={setIsDialogOpen}>
        <DialogTrigger asChild>
          <Button onClick={() => setEditingItem(null)}>
            <PlusCircle className="w-4 h-4 mr-2" /> Adicionar Local
          </Button>
        </DialogTrigger>
        <DialogContent className="max-w-2xl">
          <DialogHeader>
            <DialogTitle>{editingItem ? 'Editar' : 'Adicionar'} Local</DialogTitle>
          </DialogHeader>
          <LocalForm local={editingItem} onSave={handleSave} onCancel={() => setIsDialogOpen(false)} />
        </DialogContent>
      </Dialog>
      
      <div className="mt-4 bg-white border border-gray-200 rounded-lg overflow-hidden shadow-sm">
        <table className="min-w-full text-left text-sm">
          <thead className="bg-gray-50 border-b border-gray-200">
            <tr>
              <th className="px-4 py-2 text-gray-700">Local</th>
              <th className="px-4 py-2 text-gray-700">Código</th>
              <th className="px-4 py-2 text-gray-700">Tipo</th>
              <th className="px-4 py-2 text-gray-700">Responsável</th>
              <th className="px-4 py-2 text-gray-700">Coordenadas</th>
              <th className="px-4 py-2 text-gray-700">Ações</th>
            </tr>
          </thead>
          <tbody>
            {locais.map(item => (
              <tr key={item.id} className="border-b border-gray-100 hover:bg-gray-50">
                <td className="px-4 py-2">
                  <div className="flex items-center gap-2">
                    <MapPin className="w-4 h-4 text-emerald-600" />
                    <div>
                      <span className="font-medium text-gray-800">{item.nome}</span>
                      {item.descricao && (
                        <p className="text-xs text-gray-500 truncate max-w-48">{item.descricao}</p>
                      )}
                    </div>
                  </div>
                </td>
                <td className="px-4 py-2">
                  <span className="font-mono text-xs bg-gray-100 text-gray-700 px-2 py-1 rounded border border-gray-200">
                    {item.codigo}
                  </span>
                </td>
                <td className="px-4 py-2">
                  <span className={`text-xs font-medium capitalize ${getTipoColor(item.tipo)}`}>
                    {item.tipo.replace('_', ' ')}
                  </span>
                </td>
                <td className="px-4 py-2">
                  <div className="flex items-center gap-2">
                    <User className="w-3 h-3 text-gray-500" />
                    <span className={`text-xs font-medium ${getResponsavelColor(item.responsavel)}`}>
                      {item.responsavel}
                    </span>
                  </div>
                </td>
                <td className="px-4 py-2">
                  {item.coordenadas?.latitude && item.coordenadas?.longitude ? (
                    <span className="text-xs text-gray-600 font-mono">
                      {item.coordenadas.latitude.toFixed(4)}, {item.coordenadas.longitude.toFixed(4)}
                    </span>
                  ) : (
                    <span className="text-xs text-gray-400">-</span>
                  )}
                </td>
                <td className="px-4 py-2">
                  <Button variant="ghost" size="sm" onClick={() => { setEditingItem(item); setIsDialogOpen(true); }}>
                    <Edit className="w-4 h-4 text-gray-600 hover:text-gray-800" />
                  </Button>
                  <Button variant="ghost" size="sm" onClick={() => handleDelete(item.id)}>
                    <Trash2 className="w-4 h-4 text-red-600 hover:text-red-800" />
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
