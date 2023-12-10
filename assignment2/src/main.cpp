#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <iostream>
#include <unordered_set>
#include <vector>
#include <mpi.h>

#include "mpi_error_check.hpp"

using namespace std;

// set the maximum size of the ngram
static constexpr size_t max_pattern_len = 3;
static constexpr size_t max_dictionary_size = 128;
static_assert(max_pattern_len > 1, "The pattern must contain at least one character");
static_assert(max_dictionary_size > 1, "The dictionary must contain at least one element");

// simple class to represent a word of our dictionary
struct word {
  char ngram[max_pattern_len + 1];  // the string data, statically allocated
  size_t size = 0;                  // the string size
  size_t coverage = 0;              // the score of the word
};

// utility struct to ease working with words
struct word_coverage_lt_comparator {
  bool operator()(const word &w1, const word &w2) const { return w1.coverage < w2.coverage; }
};
struct word_coverage_gt_comparator {
  bool operator()(const word &w1, const word &w2) const { return w1.coverage > w2.coverage; }
};

// this is our dictionary of ngram
struct dictionary {
  vector<word> data;
  vector<word>::iterator worst_element;

  void add_word(word &new_word) {
    const auto coverage = new_word.coverage;
    if (data.size() < max_dictionary_size) {
      data.emplace_back(move(new_word));
      worst_element = end(data);
    } else {
      if (worst_element == end(data)) {
        worst_element = min_element(begin(data), end(data), word_coverage_lt_comparator{});
      }
      if (coverage > worst_element->coverage) {
        *worst_element = move(new_word);
        worst_element = end(data);
      }
    }
  }

  void write(ostream &out) const {
    for (const auto &word : data) {
      out << word.ngram << ' ' << word.coverage << endl;
    }
    out << flush;
  }
};

size_t count_coverage(const string &dataset, const char *ngram) {
  size_t index = 0;
  size_t counter = 0;
  const size_t ngram_size = ::strlen(ngram);
  while (index < dataset.size()) {
    index = dataset.find(ngram, index);
    if (index != string::npos) {
      ++counter;
      index += ngram_size;
    }
  }
  return counter * ngram_size;
}


int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
  // initialize
  int provided_thread_level;
  const int rc_init = MPI_Init_thread(&argc, &argv, MPI_THREAD_SINGLE, &provided_thread_level);
  exit_on_fail(rc_init);
  if(provided_thread_level < MPI_THREAD_SINGLE){
      printf("Minimum thread level not available!\n");
      return EXIT_FAILURE;
  }

  // set context
  int size;
  const int rc_size = MPI_Comm_size(MPI_COMM_WORLD, &size);
  exit_on_fail(rc_size);
  int rank;
  const int rc_rank = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  exit_on_fail(rc_rank);

  // read the whole database of SMILES and put them in a single string
  // NOTE: we can figure out which is our alphabet

  // data structures that every process need to have
  unordered_set<char> alphabet_builder;
  string database;
  int db_size = 0;
  vector<char> alphabet;
  int alphabet_size = 0;

  // only the process P0 read the input and compute initial stuff
  if(rank==0){
    cerr << "Reading the molecules from the standard input ..." << endl;

    // compute the alphabet and the database
    database.reserve(209715200);  // 200MB
    for (string line; getline(cin, line);
          /* automatically handled */) {
      for (const auto character : line) {
        alphabet_builder.emplace(character);
        database.push_back(character);
      }
    }

    // put the alphabet in a container with random access capabilities
    alphabet_builder.reserve(alphabet_builder.size());
    for_each(begin(alphabet_builder), end(alphabet_builder),
                  [&alphabet](const auto character) { alphabet.push_back(character); });

    db_size = database.size();
    alphabet_size = alphabet.size();
  }

  // send in broadcast the alphabet and its size
  const int rc_alpha_size = MPI_Bcast(&alphabet_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
  exit_on_fail(rc_alpha_size);

  alphabet.resize(alphabet_size);
  const int rc_alphabet = MPI_Bcast(alphabet.data(), alphabet.size(), MPI_CHAR, 0, MPI_COMM_WORLD);
  exit_on_fail(rc_alphabet);

  // send in broadcast the database and its size
  const int rc_db_size = MPI_Bcast(&db_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
  exit_on_fail(rc_db_size);

  database.resize(db_size);
  const int rc_database = MPI_Bcast(database.data(), database.size(), MPI_CHAR, 0, MPI_COMM_WORLD);
  exit_on_fail(rc_database);
  
  // precompute the number of permutations according to the number of characters (all processes for now compute the number of permutations)
  auto permutations = vector(max_pattern_len, alphabet.size());
  for (size_t i{1}; i < permutations.size(); ++i) {
    permutations[i] = alphabet.size() * permutations[i - size_t{1}];
  }

  // Calculate the range of ngrams to process for each MPI process and for each lenght of permutations
  size_t total_ngrams[max_pattern_len];
  size_t ngrams_per_process[max_pattern_len];
  size_t start_index[max_pattern_len];
  size_t end_index[max_pattern_len];
  for(size_t i=0; i<max_pattern_len; i++){
    total_ngrams[i] = permutations[i];
    ngrams_per_process[i] = (total_ngrams[i] + size - 1) / size;
    start_index[i] = rank * ngrams_per_process[i];
    end_index[i] = min((rank + 1) * ngrams_per_process[i] - 1, total_ngrams[i]-1);  //se metto 3 o 4 processi, cambia l'end index e quindi va out of bound
  }

  //cerr << "Process P" << rank << endl << "Total ngram1: " << total_ngrams[0] << "  Total ngram2: " << total_ngrams[1] << "  Total ngram3: " << total_ngrams[2] << endl << "Ngrams per process1: " << ngrams_per_process[0] << "  Ngrams per process2: " << ngrams_per_process[1] << "  Ngrams per process3: " << ngrams_per_process[2] << endl << "Start index1: " << start_index[0] << "  Start index2: " << start_index[1] << "  Start index3: " << start_index[2] << endl << "End index1: " << end_index[0] << "  End index2: " << end_index[1] << "  End index3: " << end_index[2] << endl;
  // declare the dictionary that holds all the ngrams with the greatest coverage of the dictionary
  dictionary result;

  for (size_t ngram_size{1}; ngram_size <= max_pattern_len; ++ngram_size) {
    vector<char> all_words;
    vector<int> all_cov;
    if(rank==0){
      all_words.resize(total_ngrams[ngram_size-1] * ngram_size);
      all_cov.resize(total_ngrams[ngram_size-1]);
    }
    vector<char> words;           // vector containing the permutations
    vector<int> cov;              // vector containing the coverage for each permutation, in order

    // Distribute work among MPI processes
    // if not P0, put into 2 vectors to send them later to P0
    if(rank==0){
      // insert directly into the dictionary
      for (size_t word_index = start_index[ngram_size-1]; word_index <= end_index[ngram_size-1]; ++word_index) {
        // compose the ngram
        word current_word;
        memset(current_word.ngram, '\0', max_pattern_len + 1);
        for (size_t character_index{0}, remaining_size = word_index; character_index < ngram_size;
                ++character_index, remaining_size /= alphabet.size()) {
            current_word.ngram[character_index] = alphabet[remaining_size % alphabet.size()];
      }
      // transform into a vector of char
      for (int i = 0; current_word.ngram[i] != '\0'; ++i) {
          words.push_back(current_word.ngram[i]);
      }

      current_word.size = ngram_size;

      // Evaluate the coverage and add the word to the dictionary
      current_word.coverage = count_coverage(database, current_word.ngram);
      cov.push_back(current_word.coverage);   // push its coverage into the cov vector
      result.add_word(current_word);
      }
    }else{
      // Don't put into the dictionary
      for (size_t word_index = start_index[ngram_size-1]; word_index <= end_index[ngram_size-1]; ++word_index) {
        // compose the ngram
        word current_word;
        memset(current_word.ngram, '\0', max_pattern_len + 1);
        for (size_t character_index{0}, remaining_size = word_index; character_index < ngram_size;
                ++character_index, remaining_size /= alphabet.size()) {
            current_word.ngram[character_index] = alphabet[remaining_size % alphabet.size()];
        }
        // Transform into a vector of char
        for (int i = 0; current_word.ngram[i] != '\0'; ++i) {
            words.push_back(current_word.ngram[i]);
        }

        current_word.size = ngram_size;

        // Evaluate the coverage and push into the cov vector
        current_word.coverage = count_coverage(database, current_word.ngram);
        cov.push_back(current_word.coverage);
      }
    }
    // Gather to P0 all the computed permutations and coverages
    //const int rc_gather_words = MPI_Gather(words.data(), ngrams_per_process[ngram_size-1]*ngram_size, MPI_CHAR, all_words.data(), ngrams_per_process[ngram_size-1]*ngram_size, MPI_CHAR, 0, MPI_COMM_WORLD);
    const int rc_gather_words = MPI_Gather(words.data(), words.size(), MPI_CHAR, all_words.data(), words.size(), MPI_CHAR, 0, MPI_COMM_WORLD);
    exit_on_fail(rc_gather_words);

    //const int rc_gather_cov = MPI_Gather(cov.data(), ngrams_per_process[ngram_size-1], MPI_INT, all_cov.data(), ngrams_per_process[ngram_size-1], MPI_INT, 0, MPI_COMM_WORLD);
    const int rc_gather_cov = MPI_Gather(cov.data(), cov.size(), MPI_INT, all_cov.data(), cov.size(), MPI_INT, 0, MPI_COMM_WORLD);
    exit_on_fail(rc_gather_cov);

    if(rank==0){
      for(size_t k=ngrams_per_process[ngram_size-1]; k<total_ngrams[ngram_size-1]; k++){
        int idx = k*ngram_size;
        word current_word;
        memset(current_word.ngram, '\0', max_pattern_len + 1);
        for(size_t h=0; h<ngram_size; h++){
            current_word.ngram[h] = all_words[idx+h]; 
        }
        current_word.size = ngram_size;
        current_word.coverage = all_cov[k];
        result.add_word(current_word);
      }
    }

    // Clear for next iteration
    all_words.clear();
    all_cov.clear();
    words.clear();
    cov.clear();
  }

  // generate the final dictionary
  // NOTE: we sort it for pretty-printing
  if(rank==0){
    cout << "NGRAM COVERAGE" << endl;
    sort(begin(result.data), end(result.data), word_coverage_gt_comparator{});
    result.write(cout);
  }

  // finalize
  const int rc_finalize = MPI_Finalize();
  exit_on_fail(rc_finalize);
  return EXIT_SUCCESS;
}
