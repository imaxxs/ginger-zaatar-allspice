#include <dirent.h>
#include <common/utility.h>
#include <crypto/prng.h>

using std::cerr;
using std::endl;

// COMMON UTILITIES
void parse_args(int argc, char **argv, char *actor, int *phase, int *batch_size, int
                *num_verifications, int *input_size, char *prover_url) {
  if (argc < 9) {
    cout<<"fewer arguments passed to start the verifier"<<endl;
    cout<<"input format: -a <v|p> [-p <1|2>] -b <num_of_batches> -r <num_verifier_repetitions> -i <input_size> [-s <prover_url>] [-o <0|1>]"<<endl;
    cout<<"starting the prover's daemon"<<endl;
  }

  if (prover_url != NULL)
    prover_url[0] = '\0';

  for (int i=1; i<argc; i++) {
    if (strcmp(argv[i], "-a") == 0 && actor != NULL)
      *actor = argv[i+1][0];
    if (strcmp(argv[i], "-p") == 0 && phase != NULL)
      *phase = atoi (argv[i+1]);
    else if (strcmp(argv[i], "-b") == 0)
      *batch_size = atoi (argv[i+1]);
    else if (strcmp(argv[i], "-r") == 0)
      *num_verifications = atoi (argv[i+1]);
    else if (strcmp(argv[i], "-i") == 0)
      *input_size = atoi (argv[i+1]);
    else if (strcmp(argv[i], "-s") == 0 && prover_url != NULL) {
      strncpy(prover_url, argv[i+1], BUFLEN-1);
      prover_url[BUFLEN-1] = '\0';
    }
  }
}

void parse_http_args(char *query_string, int *phase, int *batch_size,
                     int *batch_start, int *batch_end, int *num_verifications, int *input_size) {
  if (query_string == NULL) {
    return;
  }

  char *ptr = strtok(query_string, "=&");

  int key_id = -1;
  while (ptr != NULL) {
    if (strstr(ptr, "phase") != NULL)
      key_id = 0;
    else if (strstr(ptr, "batch_size") != NULL)
      key_id = 1;
    else if (strstr(ptr, "batch_start") != NULL)
      key_id = 2;
    else if (strstr(ptr, "batch_end") != NULL)
      key_id = 3;
    else if (strstr(ptr, "reps") != NULL)
      key_id = 4;
    else if (strstr(ptr, "m") != NULL)
      key_id = 5;
    else if (strstr(ptr, "opt") != NULL)
      key_id = 6;
    else {
      int arg = 0;
      if (key_id != -1)
        arg = atoi(ptr);

      switch (key_id) {
      case 0:
        *phase = arg;
        break;
      case 1:
        *batch_size = arg;
        break;
      case 2:
        *batch_start = arg;
        break;
      case 3:
        *batch_end = arg;
        break;
      case 4:
        *num_verifications = arg;
        break;
      case 5:
        *input_size = arg;
        break;
      }
      key_id = -1;
    }
    ptr = strtok(NULL, "=&");
  }
}

std::list<string> get_files_in_dir(char *dir_name) {
  std::list<string> files;
  DIR *dir = opendir(dir_name);
  if (dir != NULL) {
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
      string file_name(ent->d_name);
      if (!file_name.compare(".") || !file_name.compare(".."))
        continue;
      files.push_back(file_name);
    }
    closedir(dir);
  }
  files.sort();
  return files;
}

void open_file_write(ofstream& fp, const char* name, const char*folder_name) {
  char file_name[BUFLEN];

  if (folder_name == NULL)
    snprintf(file_name, BUFLEN-1, "%s/%s", FOLDER_STATE, name);
  else
    snprintf(file_name, BUFLEN-1, "%s/%s", folder_name, name);
  fp.open(file_name);
  if (!fp.is_open()) {
    cout <<"Warning: could not operate file "<<file_name << endl;
    exit(1);
  }
}

void open_file_read(ifstream& fp, const char* name, const char*folder_name) {
  char file_name[BUFLEN];
  if (folder_name == NULL)
    snprintf(file_name, BUFLEN-1, "%s/%s", FOLDER_STATE, name);
  else
    snprintf(file_name, BUFLEN-1, "%s/%s", folder_name, name);
  fp.open(file_name);
  if (!fp.is_open()) {
    cout <<"Warning: could not operate file "<<file_name << endl;
    exit(1);
  }
}

bool open_file(FILE **fp, const char *vec_name, const char *permission, const char *folder_name) {
  char file_name[BUFLEN];

  if (folder_name == NULL)
    snprintf(file_name, BUFLEN-1, "%s/%s", FOLDER_STATE, vec_name);
  else
    snprintf(file_name, BUFLEN-1, "%s/%s", folder_name, vec_name);
  *fp = fopen(file_name, permission);
  if (*fp == NULL) {
    cout <<"Warning: could not operate file "<<file_name<<" with permision "<<permission<<endl;
  }
  return *fp == NULL;
}

void convert_to_z(const int size, mpz_t *z, const mpq_t *q, const mpz_t prime) {
  for (int i = 0; i < size; i++)
    convert_to_z(z[i], q[i], prime);
}

void convert_to_z(mpz_t z, const mpq_t q, const mpz_t prime) {
  mpz_invert(z, mpq_denref(q), prime);
  mpz_mul(z, z, mpq_numref(q));
  mpz_mod(z, z, prime);
}

off_t get_file_size(const char *filename) {
  struct stat st;
  if (stat(filename, &st) == 0)
    return st.st_size;
  return -1;
}

void dump_vector_interleaved(int size, mpz_t *q, const char *vec_name, const char *folder_name) {
  FILE *fp;
  open_file(&fp, vec_name, (char *)"wb", folder_name);
  if (fp == NULL) return;

  for (int i=0; i<size/2; i++)
    mpz_out_raw(fp, q[2*i]);

  for (int i=0; i<size/2; i++)
    mpz_out_raw(fp, q[2*i+1]);

  fclose(fp);
}

bool dump_vector(int size, mpz_t *q, const char *vec_name, const char *folder_name) {
  FILE *fp;
  open_file(&fp, vec_name, (char *)"wb", folder_name);
  if (fp == NULL) return false;

  for (int i=0; i<size; i++)
    mpz_out_raw(fp, q[i]);

  fclose(fp);
  return true;
}

bool dump_vector(int size, mpq_t *q, const char *vec_name, const char *folder_name) {
  FILE *fp;
  open_file(&fp, vec_name, (char *)"wb", folder_name);

  if (!fp)
    return false;

  for (int i=0; i<size; i++) {
    mpz_out_raw(fp, mpq_numref(q[i]));
    mpz_out_raw(fp, mpq_denref(q[i]));
  }
  fclose(fp);
  return true;
}

/**
 * Dump just the numerators of a vector of rational numbers, writing them out as 32 bit ints.
 */
void dump_binary_nums(int size, mpq_t *q, const char *vec_name, const char *folder_name) {
  FILE *fp;
  int32_t buf[1] = {0};
  open_file(&fp, vec_name, (char *)"wb", folder_name);
  for (int i=0; i<size; i++) {
    buf[0] = mpz_get_si(mpq_numref(q[i]));
    fwrite(buf, sizeof(int32_t), 1, fp);
  }
  fclose(fp);
}

void dump_scalar(mpz_t q, char *scalar_name, const char *folder_name) {
  FILE *fp;
  open_file(&fp, scalar_name, (char *)"wb", folder_name);
  if (fp == NULL) return;
  mpz_out_raw(fp, q);
  fclose(fp);
}

/*
 * Dump an entire array of scalars. Useful when there are multiple functions in
 * the prover.
 *
 * Vectors will be stored with the following name:
 *      f<num_func>_<suffix>
 * num_func is an integer in [0, n).
 */
void dump_scalar_array(int n, mpz_t *scalars, const char *suffix, char *folder_name) {
  char vec_name[BUFLEN];

  for (int i = 0; i < n; i++) {
    snprintf(vec_name, sizeof(vec_name), "f%d_%s", i, suffix);
    dump_scalar(scalars[i], vec_name, folder_name);
  }
}


void load_prng_seed(int bytes_key, void *key, int bytes_iv, void *iv) {
  FILE *fp;
  open_file(&fp, "seed_decommit_queries", (char *)"r", NULL);
  if (((size_t)bytes_key != fread(key, 1, bytes_key, fp)) || ((size_t)bytes_iv != fread(iv, 1, bytes_iv, fp)))
    cerr<<"Error loading the seed for the decommitment queries"<<endl;
  fclose(fp);
}

void dump_prng_seed(int bytes_key, void *key, int bytes_iv, void *iv) {
  FILE *fp;
  open_file(&fp, "seed_decommit_queries", (char *)"w", NULL);
  fwrite(key, 1, bytes_key, fp);
  fwrite(iv, 1, bytes_iv, fp);
  fclose(fp);
}

void load_vector(int size, uint32_t *vec, const char *full_file_name) {
  FILE *fp = fopen(full_file_name, "r");
  if (fp == NULL) {
    cout<<"Cannot read "<<full_file_name<<endl;
    exit(1);
  }

  int ret;
  for (int i=0; i < size; i++) {
    ret = fscanf(fp, "%d ", &vec[i]);
    if (ret <= 0)
      break;
  }
  fclose(fp);
}

void load_vector(int size, mpz_t *q, const char *vec_name, const char *folder_name) {
  FILE *fp;
  open_file(&fp, vec_name, (char *)"rb", folder_name);
  if (fp == NULL) return;
  for (int i=0; i<size; i++)
    mpz_inp_raw(q[i], fp);
  fclose(fp);
}

void load_vector(int size, mpq_t *q, const char *vec_name, const char *folder_name) {
  FILE *fp;
  open_file(&fp, vec_name, (char *)"rb", folder_name);
  for (int i=0; i<size; i++) {
    mpz_inp_raw(mpq_numref(q[i]), fp);
    mpz_inp_raw(mpq_denref(q[i]), fp);
  }
  fclose(fp);
}

void load_scalar(mpz_t q, const char *scalar_name, const char *folder_name) {
  FILE *fp;
  open_file(&fp, scalar_name, (char *)"rb", folder_name);
  if (fp == NULL) return;
  mpz_inp_raw(q, fp);
  fclose(fp);
}

/*
 * Load an entire array of scalars. Useful when there are multiple functions in
 * the prover.
 *
 * Vectors will be stored with the following name:
 *      f<num_func>_<suffix>
 * num_func is an integer in [0, n).
 */
void load_scalar_array(int n, mpz_t *scalars, const char *suffix, char *folder_name) {
  char vec_name[BUFLEN];

  for (int i = 0; i < n; i++) {
    snprintf(vec_name, sizeof(vec_name), "f%d_%s", i, suffix);
    load_scalar(scalars[i], vec_name, folder_name);
  }
}

void load_txt_scalar(mpz_t q, const char *scalar_name, const char *folder_name) {
  FILE *fp;
  open_file(&fp, scalar_name, (char *)"rb", folder_name);
  if (fp == NULL) return;
  mpz_inp_str(q, fp, 10);
  fclose(fp);
}

void clear_scalar(mpz_t s) {
  mpz_clear(s);
}

void clear_scalar(mpq_t s) {
  mpq_clear(s);
}

void clear_vec(int size, mpz_t *arr) {
  for (int i=0; i<size; i++)
    mpz_clear(arr[i]);
}

void clear_vec(int size, mpq_t *arr) {
  for (int i=0; i<size; i++)
    mpq_clear(arr[i]);
}

void alloc_init_vec(mpz_t **arr, uint32_t size) {
  *arr = new mpz_t[size];
  for (uint32_t i=0; i<size; i++)
    alloc_init_scalar((*arr)[i]);
}

void alloc_init_vec(mpq_t **arr, uint32_t size) {
  *arr = new mpq_t[size];
  for (uint32_t i=0; i<size; i++) {
    alloc_init_scalar((*arr)[i]);
  }
}

void
alloc_init_vec_array(mpz_t ***array, const uint32_t n, const uint32_t vecSize) {
  mpz_t **newArray = new mpz_t*[n];

  for (uint32_t i = 0; i < n; i++)
    alloc_init_vec(&newArray[i], vecSize);

  *array = newArray;
}

void alloc_init_scalar(mpz_t s) {
  mpz_init2(s, INIT_MPZ_BITS);
  //mpz_init(s);
  mpz_set_ui(s, 0);
}

void alloc_init_scalar(mpq_t s) {
  mpq_init(s);
  mpq_set_ui(s, 0, 1);
}

void
clear_del_vec(mpz_t* vec, const uint32_t n) {
  for (uint32_t i = 0; i < n; i++)
    mpz_clear(vec[i]);
  delete[] vec;
}

void
clear_del_vec_array(mpz_t **array, const uint32_t n, const uint32_t vecSize) {
  for (uint32_t i = 0; i < n; i++) {
    clear_del_vec(array[i], vecSize);
  }
  delete[] array;
}

void
clear_del_vec(mpq_t* vec, const uint32_t n) {
  for (uint32_t i = 0; i < n; i++)
    mpq_clear(vec[i]);
  delete[] vec;
}

bool
modIfNeeded(mpz_t val, const mpz_t prime, int slack)
{
  if (mpz_sizeinbase(val, 2) >= slack * mpz_sizeinbase(prime, 2) - 1)
  {
    mpz_mod(val, val, prime);
    return true;
  }
  else
  {
    return false;
  }
}

bool
modIfNeeded(mpq_t val, const mpz_t prime, int slack)
{
  bool changed = false;
  changed = modIfNeeded(mpq_numref(val), prime, slack) || changed;
  changed = modIfNeeded(mpq_denref(val), prime, slack) || changed;

  if (changed)
    mpq_canonicalize(val);

  return changed;
}


void print_matrix(mpz_t *matrix, uint32_t num_rows, uint32_t num_cols, string name) {
  cout << "\n" << name << " =" << endl;
  for (uint32_t i = 0; i < num_rows*num_cols; i++) {
    gmp_printf("%Zd ", matrix[i]);
    if (i % num_cols == num_cols - 1)
      gmp_printf("\n");
  }
  cout << endl;
}

void print_sq_matrix(mpz_t *matrix, uint32_t size, string name) {
  print_matrix(matrix, size, size, name);
}

void* aligned_malloc(size_t size) {
  void* ptr = malloc(size + PAGESIZE);

  if (ptr) {
    void* aligned = (void*)(((long)ptr + PAGESIZE) & ~(PAGESIZE - 1));
    ((void**)aligned)[-1] = ptr;
    return aligned;
  } else
    return NULL;
}

bool verify_conversion_to_z(size_t size, mpz_t *z, mpq_t *q, mpz_t prime) {
  mpz_t temp;
  mpz_init(temp);

  for (size_t i = 0; i < size; i++) {
    mpz_mul(temp, z[i], mpq_denref(q[i]));
    mpz_sub(temp, temp, mpq_numref(q[i]));
    mpz_mod(temp, temp, prime);
    if (mpz_cmp_ui(temp, 0) != 0) {
      //cout<<"ERROR: Conversion test failed."<<endl;
      return false;
    }
  }

  mpz_clear(temp);

  return true;
}

void print_stats(const char *operation, vector<double> s) {
  double min = 0, max = 0;
  double avg = 0;

  sort(s.begin(), s.begin()+s.size());

  int start_index = 0;
  int end_index = s.size();
  int count = 0;

  for (long i=start_index; i<end_index; i++) {
    if (s[i] < min || min == 0)
      min = s[i];

    if (s[i] > max)
      max = s[i];

    avg = avg + s[i];
    count = count + 1;
  }

  avg = avg/count;

  double sd = 0;
  for (long i=start_index; i<end_index; i++) {
    sd = sd + (s[i] - avg) * (s[i] - avg);
  }

  sd = sqrt(sd/count);
  cout<<"* "<<operation<<"_min "<<min<<endl;
  cout<<"* "<<operation<<"_max "<<max<<endl;
  cout<<"* "<<operation<<"_avg "<<avg<<endl;
  cout<<"* "<<operation<<"_sd "<<sd<<endl<<endl;
}
