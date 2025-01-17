#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Peregrine.hh"

#include "Domain.hh"

#include <cstdlib>

int main(int argc, char *argv[])
{
  
  if (argc < 4)
  {
    std::cerr << "USAGE: " << argv[0] << " <data graph> <max size> <support threshold> [vertex-induced] [# threads]" << std::endl;
    return -1;
  }

  const std::string data_graph_name(argv[1]);
  uint32_t k = std::stoi(argv[2]);

  uint64_t threshold = std::stoi(argv[3]);

  //size_t nthreads = std::thread::hardware_concurrency(); // 64
  size_t nthreads = 1;
  bool extension_strategy = Peregrine::PatternGenerator::EDGE_BASED;

  uint32_t step = 1;

  const auto view = [](auto &&v) { return v.get_support(); };

  std::vector<uint64_t> supports;
  std::vector<Peregrine::SmallGraph> freq_patterns;

  std::cout << k << "-FSM with threshold " << threshold << std::endl;
  //std::cout << "Sleep before dg" << std::endl;

  Peregrine::DataGraph dg(data_graph_name);
  //return 0;
  //std::cout << "Sleep after dg" << std::endl;
  //sleep(600);

  // initial discovery
  auto t1 = utils::get_timestamp();
  {
    const auto process = [](auto &&a, auto &&cm) {
      uint32_t merge = cm.pattern[0] == cm.pattern[1] ? 0 : 1;
      a.map(cm.pattern, std::make_pair(cm.mapping, merge));
    };

    std::vector<Peregrine::SmallGraph> patterns = {Peregrine::PatternGenerator::star(2)};
    patterns.front().set_labelling(Peregrine::Graph::DISCOVER_LABELS);
    //std::cout << "Before ini match" << std::endl;
    //sleep(600);
    auto psupps = Peregrine::match<Peregrine::Pattern, DiscoveryDomain<1>, Peregrine::AT_THE_END, Peregrine::UNSTOPPABLE>(dg, patterns, nthreads, process, view);
    //std::cout << "after match" << std::endl;
    //sleep(600);
    for (const auto &[p, supp] : psupps)
    {
      if (supp >= threshold)
      {
        freq_patterns.push_back(p);
        supports.push_back(supp);
      }
    }
  }
    
  //std::cout << "before extend" << std::endl;
  //sleep(600);
  std::vector<Peregrine::SmallGraph> patterns = Peregrine::PatternGenerator::extend(freq_patterns, extension_strategy);

  //std::cout << "after init extend" << std::endl;
  //sleep(600);


  const auto process = [](auto &&a, auto &&cm) {
    a.map(cm.pattern, cm.mapping);
  };

  while (step < k && !patterns.empty())
  {
    std::cout << "step=" << step << std::endl;
    freq_patterns.clear();
    supports.clear();

    auto psupps = Peregrine::match<Peregrine::Pattern, Domain, Peregrine::AT_THE_END, Peregrine::UNSTOPPABLE>(dg, patterns, nthreads, process, view, 1);
   
    
    
    std::cout << "After match!" << std::endl;
    std::cout << "psupps.size()=" << psupps.size() << std::endl;
    std::cout << "total size of psupps = " << sizeof(psupps) + psupps.size()*sizeof(psupps[0]) << std::endl;
    
    //sleep(600);
    
    //int counter = 0;
    for (const auto &[p, supp] : psupps)
    {
      if (supp >= threshold)
      {
        //++counter;
        freq_patterns.push_back(p);
        supports.push_back(supp);
      }
    }

    //std::cout << "counter=" << counter << std::endl;

    /*
    std::cout << "Bf extend!" << std::endl;
    std::cout << "freq_patterns.size()=" << freq_patterns.size() << std::endl;
    std::cout << "total size of freq_patterns = " << sizeof(freq_patterns) + freq_patterns.size()*sizeof(freq_patterns[0]) << std::endl;
    std::cout << "supports.size()=" << supports.size() << std::endl;
    std::cout << "total size of supports = " << sizeof(supports) + supports.size()*sizeof(supports[0]) << std::endl;
    sleep(600);
    */

    patterns = Peregrine::PatternGenerator::extend(freq_patterns, extension_strategy);
    std::cout << "extend finishes!" << std::endl;
    step += 1;
  }
  auto t2 = utils::get_timestamp();

  std::cout << freq_patterns.size() << " frequent patterns: " << std::endl;
  for (uint32_t i = 0; i < freq_patterns.size(); ++i)
  {
    std::vector<uint32_t> plabels = freq_patterns[i].get_labels();
    for(uint32_t j=0;j<plabels.size();++j) {
      Peregrine::multilabel temp_ml = dg.get_multilabel(plabels[j]);
      freq_patterns[i].set_multilabel(j, temp_ml);
    }

    std::cout << freq_patterns[i].to_string() << ": " << supports[i] << std::endl;
  }

  std::cout << "finished in " << (t2-t1)/1e6 << "s" << std::endl;
  
  return 0;
}
