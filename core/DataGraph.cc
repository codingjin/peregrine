#include <iostream>
#include <memory>

#include <string>
#include <cstring>
#include <algorithm>
#include <unordered_set>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <filesystem>

#include "utils.hh"
#include "Graph.hh"
#include "DataGraph.hh"

namespace Peregrine
{
  void DataGraph::from_smallgraph(const SmallGraph &pp)
  {
    SmallGraph p(pp);
    graph_in_memory = std::make_unique<uint32_t[]>(2 * p.num_true_edges());
    data_graph = std::make_unique<adjlist[]>(p.num_vertices()+1);

    uint32_t cursor = 0;
    /*
    for (uint32_t v = 1; v <= p.num_vertices(); ++v) {
      std::sort(p.true_adj_list.at(v).begin(), p.true_adj_list.at(v).end());

      std::memcpy(&graph_in_memory[cursor], &p.true_adj_list.at(v)[0], p.true_adj_list.at(v).size() * sizeof(uint32_t));
      data_graph[v-1].ptr = &graph_in_memory[cursor];
      data_graph[v-1].length = p.true_adj_list.at(v).size();
      cursor += p.true_adj_list.at(v).size();
    }
    */

   for (uint32_t i=1;i<=p.num_vertices();++i) {
     if (p.get_multilabel(i).l0 == uint32_t(-1)) {
       data_graph[i-1].ptr = nullptr;
       data_graph[i-1].length = 0;
     }else {
       std::sort(p.true_adj_list.at(i).begin(), p.true_adj_list.at(i).end());
       std::memcpy(&graph_in_memory[cursor], &p.true_adj_list.at(i)[0], p.true_adj_list.at(i).size() * sizeof(uint32_t));
       data_graph[i-1].ptr = &graph_in_memory[cursor];
       data_graph[i-1].length = p.true_adj_list.at(i).size();
       cursor += p.true_adj_list.at(i).size();
     }
     
   }

    //vertex_count = p.num_vertices();
    vertex_count = p.num_vertices();
    edge_count = p.num_true_edges();

    labels = std::make_unique<uint32_t[]>(vertex_count+1);
    if (p.labelling == Graph::LABELLED)
    {
      labelled_graph = true;

      multilabels = std::make_unique<multilabel[]>(vertex_count + 1);

      for (uint32_t u = 0; u < vertex_count; ++u)
      {
        labels[u+1] = p.labels[u];
        multilabels[u+1] = p.multilabels[u];
      }

      /*
      uint32_t min_label = *std::min_element(p.labels.cbegin(), p.labels.cend());
      uint32_t max_label = *std::max_element(p.labels.cbegin(), p.labels.cend());
      */

      uint32_t min_label, max_label;
      for (uint32_t i=1;i<=vertex_count;++i) {
        if (multilabels[i].l0 != uint32_t(-1)) {
          min_label = labels[i];
          max_label = labels[i];
          for (uint32_t j=i+1;j<=vertex_count;++j) {
            if (multilabels[j].l0 != uint32_t(-1)) {
              if (labels[j] < min_label)  min_label = labels[j];
              else if (labels[j] > max_label) max_label = labels[j];
            }
          }
          break;
        }
      }

      /*
      std::cout << "min_label=" << min_label << " max_label=" << max_label << std::endl;
      for (uint32_t i=1;i<=vertex_count;++i) {
        if (labels[i]>=min_label && labels[i]<=max_label) std::cout << labels[i] << std::endl;
      }
      */

      label_range = std::make_pair(min_label, max_label);
    }
    ids = std::make_unique<uint32_t[]>(vertex_count+1);
    std::iota(&ids[0], &ids[vertex_count+1], 0);
  }

  DataGraph::DataGraph(const SmallGraph &p)
  {
    from_smallgraph(p);
  }

  DataGraph::DataGraph(const std::string &path_str)
  {
    std::filesystem::path data_graph_path(path_str);
    if (std::filesystem::is_directory(data_graph_path))
    {
      std::filesystem::path data_path(data_graph_path / "data.bin");
      if (!std::filesystem::exists(data_path))
      {
        std::cerr << "ERROR: Data graph could not be opened." << std::endl;
        exit(1);
      }

      std::ifstream input_graph(data_path, std::ios::binary);
      input_graph.read(reinterpret_cast<char *>(&vertex_count), sizeof(vertex_count));
      input_graph.read(reinterpret_cast<char *>(&edge_count), sizeof(edge_count));

      // don't count the header (two 32-bit integers)
      uint64_t file_size = std::filesystem::file_size(data_path);
      assert(file_size % 4 == 0);
      uint64_t data_count = file_size / 4;
      graph_in_memory = std::make_unique<uint32_t[]>(data_count - 2);

      uint64_t curr_read_offset = 0;
      uint64_t read_batch_size = 2147479552;
      while (curr_read_offset/4 <= data_count - 2)
      {
        input_graph.read(reinterpret_cast<char *>(graph_in_memory.get() + curr_read_offset/4), std::min(read_batch_size, (data_count-2)*4-curr_read_offset));
        curr_read_offset += read_batch_size;
      }

      data_graph = std::make_unique<adjlist[]>(vertex_count);

      uint32_t cursor = 0;
      for (uint32_t i = 0; i < vertex_count; i++)
      {
        data_graph[i].length = graph_in_memory[cursor];
        data_graph[i].ptr = &graph_in_memory[++cursor];
        cursor += data_graph[i].length;
      }

      labels = std::make_unique<uint32_t[]>(vertex_count+1);
      std::ifstream labels_file(data_graph_path / "labels.bin", std::ios::binary);
      if (labels_file)
      {
        uint32_t min_label = UINT_MAX;
        uint32_t max_label = 0;
        labelled_graph = true;

        for (uint32_t i = 0; i < vertex_count; ++i)
        {
          uint32_t buf[2];
          labels_file.read(reinterpret_cast<char *>(buf), 2*sizeof(uint32_t));

          uint32_t v = buf[0];
          uint32_t label = buf[1];
          min_label = std::min(min_label, label);
          max_label = std::max(max_label, label);
          labels[v] = label;
        }

        label_range = {min_label, max_label};
      }

      {
        std::ifstream ids_file(data_graph_path / "ids.bin", std::ios::binary);

        if (!ids_file)
        {
          std::cerr << "WARNING: Could not open ID file for data graph." << std::endl;
        }
        else
        {
          ids = std::make_unique<uint32_t[]>(vertex_count+1);
          for (uint32_t i = 1; i <= vertex_count; ++i)
          {
            ids_file.read(reinterpret_cast<char *>(&ids[i]), sizeof(uint32_t));
          }
        }
      }
    }
    else if (std::filesystem::is_regular_file(data_graph_path))
    {
      from_smallgraph(path_str);
    }
    else
    {
      std::cerr << "ERROR: Data graph could not be opened." << std::endl;
      exit(1);
    }
  }

  DataGraph::DataGraph(DataGraph &&other)
    : rbi(other.rbi),
      vertex_count(other.vertex_count),
      edge_count(other.edge_count),
      forest_count(other.forest_count),
      labelled_graph(other.labelled_graph),
      labels(std::move(other.labels)),
      label_range(other.label_range),
      ids(std::move(other.ids)),
      data_graph(std::move(other.data_graph)),
      graph_in_memory(std::move(other.graph_in_memory)),
      known_labels(other.known_labels),
      debug(0)
  {
    other.vertex_count = 0;
    other.edge_count = 0;
  }

  void DataGraph::set_debug()
  {
    debug = 1;
  }

  void DataGraph::clear_debug()
  {
    debug = 0;
  }

  uint32_t DataGraph::get_debug() const
  {
    return debug;
  }

  void DataGraph::set_rbi(const AnalyzedPattern &new_rbi)
  {
    rbi = new_rbi;
    forest_count = rbi.vgs.size();
    if (rbi.labelling_type() == Graph::PARTIALLY_LABELLED)
    {
      new_label = std::distance(rbi.query_graph.labels.cbegin(),
          std::find(rbi.query_graph.labels.cbegin(),
            rbi.query_graph.labels.cend(), static_cast<uint32_t>(-1))
          );
    }
  }

  void DataGraph::set_known_labels(const std::vector<SmallGraph> &patterns)
  {
    for (const auto &p : patterns)
    {
      known_labels.insert(p.labels.cbegin(), p.labels.cend());
    }
  }

  void DataGraph::set_known_labels(const std::vector<uint32_t> &labels)
  {
    known_labels.insert(labels.cbegin(), labels.cend());
  }

  bool DataGraph::known_label(const uint32_t label) const
  {
    return known_labels.contains(label);
  }

  const std::vector<uint32_t> &DataGraph::get_upper_bounds(uint32_t v) const
  {
    return rbi.upper_bounds[v];
  }

  const std::vector<uint32_t> &DataGraph::get_lower_bounds(uint32_t v) const
  {
    return rbi.lower_bounds[v];
  }

  const std::pair<uint32_t, uint32_t> &DataGraph::get_label_range() const
  {
    return label_range;
  }

  const adjlist &DataGraph::get_adj(uint32_t v) const
  {
    return data_graph[v-1];
  }

  const SmallGraph &DataGraph::get_vgs(unsigned vgsi) const
  {
    return rbi.vgs[vgsi];
  }

  uint32_t DataGraph::original_id(uint32_t v) const
  {
    return ids[v];
  }

  const SmallGraph &DataGraph::get_pattern() const {
    return rbi.query_graph;
  }

  const std::vector<std::vector<uint32_t>> &DataGraph::get_qs(unsigned vgsi) const
  {
    return rbi.qs[vgsi];
  }

  uint32_t DataGraph::label(uint32_t dv) const
  {
    return labels[dv];
  }

  const multilabel &DataGraph::get_multilabel(const uint32_t &l) const
  {
    for (uint32_t i=1;i<vertex_count;++i) {
      if (labels[i]==l) return multilabels[i];
    }

    std::cerr << "label value not found in original DataGraph!" << std::endl;
    throw std::invalid_argument("Invalid label value to map multilabel value!");
  }

  uint32_t DataGraph::vmap_at(unsigned vgsi, uint32_t v, unsigned qsi) const
  {
    return rbi.vmap[vgsi][v][qsi];
  }

  const std::vector<uint32_t> &DataGraph::get_qo(uint32_t vgsi) const
  {
    return rbi.qo_book[vgsi];
  }

  uint32_t DataGraph::get_vgs_count() const
  {
    return rbi.vgs.size();
  }

  uint32_t DataGraph::get_vertex_count() const
  {
    return vertex_count;
  }

  uint64_t DataGraph::get_edge_count() const
  {
    return edge_count;
  }
}
