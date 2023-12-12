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

// Set the maximum size of the ngram
static constexpr size_t max_pattern_len = 3;
static constexpr size_t max_dictionary_size = 128;
static_assert(max_pattern_len > 1, "The pattern must contain at least one character");
static_assert(max_dictionary_size > 1, "The dictionary must contain at least one element");

// Simple class to represent a word of our dictionary
struct word {
  char ngram[max_pattern_len + 1];  // the string data, statically allocated
  size_t size = 0;                  // the string size
  size_t coverage = 0;              // the score of the word
};

// Utility struct to ease working with words
struct word_coverage_lt_comparator {
  bool operator()(const word &w1, const word &w2) const { return w1.coverage < w2.coverage; }
};
struct word_coverage_gt_comparator {
  bool operator()(const word &w1, const word &w2) const { return w1.coverage > w2.coverage; }
};

// This is our dictionary of ngram
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

// Create the MPI_Datatype for the word structure
void createMPIWordType(MPI_Datatype* mpiWordType) {
  int blocklengths[3] = {max_pattern_len + 1, 1, 1};
  MPI_Datatype types[3] = {MPI_CHAR, MPI_UNSIGNED_LONG, MPI_UNSIGNED_LONG};
  MPI_Aint offsets[3];

  offsets[0] = offsetof(word, ngram);
  offsets[1] = offsetof(word, size);
  offsets[2] = offsetof(word, coverage);

  MPI_Type_create_struct(3, blocklengths, offsets, types, mpiWordType);
  MPI_Type_commit(mpiWordType);
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {

  // Initialize
  int provided_thread_level;
  const int rc_init = MPI_Init_thread(&argc, &argv, MPI_THREAD_SINGLE, &provided_thread_level);
  exit_on_fail(rc_init);
  if(provided_thread_level < MPI_THREAD_SINGLE){
      printf("Minimum thread level not available!\n");
      return EXIT_FAILURE;
  }

  // Set context
  int size;
  const int rc_size = MPI_Comm_size(MPI_COMM_WORLD, &size);
  exit_on_fail(rc_size);
  int rank;
  const int rc_rank = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  exit_on_fail(rc_rank);

  // Data structures that every process need to have
  unordered_set<char> alphabet_builder;
  string database;
  int db_size = 0;
  vector<char> alphabet;
  int alphabet_size = 0;

  // Only the process P0 read the input and compute initial stuff
  // Using a producer/consumer style
  if(rank==0){
    cerr << "Reading the molecules from the standard input ..." << endl;
    // Read the whole database of SMILES and put them in a single string
    // NOTE: we can figure out which is our alphabet
    // Compute the alphabet and the database
    database.reserve(209715200);  // 200MB
    for (string line; getline(cin, line);
          /* automatically handled */) {
      for (const auto character : line) {
        alphabet_builder.emplace(character);
        database.push_back(character);
      }
    }

    // Put the alphabet in a container with random access capabilities
    alphabet_builder.reserve(alphabet_builder.size());
    for_each(begin(alphabet_builder), end(alphabet_builder),
                  [&alphabet](const auto character) { alphabet.push_back(character); });

    db_size = database.size();
    alphabet_size = alphabet.size();
  }

  // Send in broadcast the alphabet and its size
  const int rc_alpha_size = MPI_Bcast(&alphabet_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
  exit_on_fail(rc_alpha_size);

  alphabet.resize(alphabet_size);
  const int rc_alphabet = MPI_Bcast(alphabet.data(), alphabet.size(), MPI_CHAR, 0, MPI_COMM_WORLD);
  exit_on_fail(rc_alphabet);

  // Send in broadcast the database and its size
  const int rc_db_size = MPI_Bcast(&db_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
  exit_on_fail(rc_db_size);

  database.resize(db_size);
  const int rc_database = MPI_Bcast(database.data(), database.size(), MPI_CHAR, 0, MPI_COMM_WORLD);
  exit_on_fail(rc_database);
  
  // Precompute the number of permutations according to the number of characters
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
    end_index[i] = min((rank + 1) * ngrams_per_process[i], total_ngrams[i]) - 1;
  }
 
  // Declare the dictionary that holds all the ngrams with the greatest coverage of the dictionary
  dictionary result;

  MPI_Datatype mpiWordType;
  createMPIWordType(&mpiWordType);

  // Compute the ngrams and their coverage
  for (size_t ngram_size{1}; ngram_size <= max_pattern_len; ++ngram_size) {
    if(rank==0){
      cerr << "Computing ngrams of size " << ngram_size << " and their coverage ..." << endl;
    }
    vector<word> all_words;
    vector<word> words;           

    // Distribute work among MPI processes
    for (size_t word_index = start_index[ngram_size-1]; word_index <= end_index[ngram_size-1]; ++word_index) {
      // Compose the ngram
      word current_word;
      memset(current_word.ngram, '\0', max_pattern_len + 1);
      for (size_t character_index{0}, remaining_size = word_index; character_index < ngram_size;
              ++character_index, remaining_size /= alphabet.size()) {
          current_word.ngram[character_index] = alphabet[remaining_size % alphabet.size()];
      }

      current_word.size = ngram_size;
      current_word.coverage = count_coverage(database, current_word.ngram);
      words.push_back(current_word);
    }

    /*
    * Uncomment this part to skip useless work (sending all the ngrams computed by each process),
    * but this leads to an output file that can be different in the order of ngrams depending on 
    * the number of processors used
    */
    // Sort and remove whats beyond max_dictionary_size, keep only the ngrams with highest coverage
    /*std::sort(words.begin(), words.end(), compareByCoverage);
    if (words.size() > max_dictionary_size) {
      words.erase(words.begin() + max_dictionary_size, words.end());
    }*/

    if(rank==0){
      all_words.resize(words.size() * size);
    }

    // Gather to P0 all the computed ngrams and respective coverages
    const int rc_gather_words = MPI_Gather(words.data(), words.size(), mpiWordType, all_words.data(), words.size(), mpiWordType, 0, MPI_COMM_WORLD);
    exit_on_fail(rc_gather_words);

    // Only P0 populates the dictionary
    if(rank==0){
      for(auto w : all_words){
        result.add_word(w);
      }
    }
  }

  // Generate the final dictionary
  // NOTE: we sort it for pretty-printing
  if(rank==0){
    cout << "NGRAM COVERAGE" << endl;
    sort(begin(result.data), end(result.data), word_coverage_gt_comparator{});
    result.write(cout);
    cerr << "Final dictionary printed in the output file" << endl;
  }

  MPI_Type_free(&mpiWordType);

  // Finalize
  const int rc_finalize = MPI_Finalize();
  exit_on_fail(rc_finalize);
  return EXIT_SUCCESS;
}
