// Estimation of K via stick-breaking process without marginalization

#include <RcppArmadilloExtensions/sample.h>
#include <RcppDist.h>
// [[Rcpp::depends(RcppArmadillo, RcppDist)]]

using namespace Rcpp;
using namespace arma;

static Mat<double> fisher(Mat<double> W);
static double logsumexp(Col<double> x);
//static Mat<double> inv_fisher(Mat<double> W_inv);

// [[Rcpp::export]]
Rcpp::List auto_WSBM(Mat<double> W, int K_max, double eta0, bool store) {
  
  int iter = 1000, burn = 0.5*iter, K = K_max;
  Mat<double> W_f = fisher(W);
  
  // Initialization
  int n = W.n_rows, l = 0;
  Col<int> n_k(K, fill::zeros);
  Mat<int> matrix_n(K, K, fill::zeros);
  Mat<double> W_sum(K, K, fill::zeros);
  Mat<double> W_sum_sq(K, K, fill::zeros);
  Mat<double> W_sum_sq_2(K, K, fill::zeros);
  Mat<double> mu(K, K, fill::zeros);
  Mat<double> Var(K, K);
  Var.fill(0.1);
  double LogL = 0.0;
  double SS0 = 0.1, nu0 = 10.0, mu0 = 0.0, n0 = 1.0;
  int K_start = randi(1, distr_param(1, K))(0);
  Col<int> z = randi(n, distr_param(0, K_start - 1));
  //Col<int> z(n, fill::zeros);
  Col<int> z_temp = z;
  
  Col<double> sampler(K, fill::zeros);
  for(int k = 0; k < K; k++){
    sampler(k) = k;
  }
  
  // DP initialization
  
  Col<double> log_beta(K, fill::zeros);
  Col<double> log_alpha(K, fill::zeros);
  Col<double> gamma(K, fill::zeros);
  
  // Store
  //Mat<int> z_store(n*iter, n, fill::zeros);
  //Mat<int> z_store(iter - burn, n, fill::zeros);
  Mat<int> z_store(iter, n, fill::zeros);
  //Col<double> logprob_store(iter, fill::zeros);
  Cube<double> mu_store(K, K, iter - burn, fill::zeros);
  Cube<double> var_store(K, K, iter - burn, fill::zeros);
  Cube<double> n_k_store(K, K, iter, fill::zeros);
  //Mat<double> alpha_mat(iter, K, fill::zeros);
  //Var.fill(1);
  
  // Temp
  int count = 0;//, count_2 = 0;
  Col<double> logprob_temp(K, fill::zeros);
  Col<double> prob_temp(K, fill::zeros);
  NumericVector prob_temp_2(K);
  Col<double> logpost_store(iter, fill::zeros);
  
  for(int k = 0; k < K; k++){
    n_k(k) = size(z.elem(find(z == k)))(0);
  }
  for(int k = 0; k < K; k++){
    for(int kk = k; kk < K; kk++){
      
      if(k == kk){
        matrix_n(k, kk) = n_k(k) * (n_k(kk) - 1)/2;
        W_sum(k, kk) = sum(vectorise(W_f.elem(find(z == k), find(z == kk))))/2;
        W_sum_sq(k, kk) = sum(pow(vectorise(W_f.elem(find(z == k), find(z == kk))), 2))/2;
        if(matrix_n(k, kk) > 0){
          W_sum_sq_2(k, kk) = W_sum_sq(k, kk) - pow(W_sum(k, kk), 2)/matrix_n(k, kk);
          Var(k, kk) = 1/rgamma(1, (matrix_n(k, kk) + nu0)/2,
              2/(SS0 + W_sum_sq_2(k, kk) + ((n0*matrix_n(k, kk))/(n0 + matrix_n(k, kk)))*pow(W_sum(k, kk)/matrix_n(k, kk) - mu0, 2)))(0);
        }else{
          W_sum_sq_2(k, kk) = 0.0;
          Var(k, kk) = SS0;
        }
        
      }
      else{
        matrix_n(k, kk) = n_k(k) * n_k(kk);
        W_sum(k, kk) = sum(vectorise(W_f.elem(find(z == k), find(z == kk))));
        W_sum_sq(k, kk) = sum(pow(vectorise(W_f.elem(find(z == k), find(z == kk))), 2));
        if(matrix_n(k, kk) > 0){
          W_sum_sq_2(k, kk) = W_sum_sq(k, kk) - pow(W_sum(k, kk), 2)/matrix_n(k, kk);
          Var(k, kk) = 1/rgamma(1, (matrix_n(k, kk) + nu0)/2,
              2/(SS0 + W_sum_sq_2(k, kk) + ((n0*matrix_n(k, kk))/(n0 + matrix_n(k, kk)))*pow(W_sum(k, kk)/matrix_n(k, kk) - mu0, 2)))(0);
        }else{
          W_sum_sq_2(k, kk) = 0.0;
          Var(k, kk) = SS0;
        }
      }
    }
  }
  
  for(int k = 0; k < K; k++){
    for(int kk = k; kk < K; kk++){
      mu(k, kk) = rnorm(1, (W_sum(k, kk) + n0*mu0)/(matrix_n(k, kk) + n0), sqrt(Var(k, kk)/(matrix_n(k, kk) + n0)))(0);
      if(store){
        LogL = LogL + (-1.0*matrix_n(k, kk)/2.0) * log(Var(k, kk)) - W_sum_sq(k, kk)/(2.0*Var(k, kk))
        + mu(k, kk)*W_sum(k, kk)/Var(k, kk) - matrix_n(k, kk)*pow(mu(k, kk), 2.0)/(2.0*Var(k, kk));
        LogL = LogL - 0.5*log(Var(k, kk)/n0) - (n0/(2.0*Var(k, kk))) * pow(mu(k, kk) - mu0, 2.0);
        LogL = LogL - (nu0/2 + 1) * log(Var(k, kk)) + SS0/(2*Var(k, kk));
      }
    }
  }
  
  

  
  //Rcout<<sum(vectorise(A.elem(find(z == 1), find(z == 1))))<<"\n";
  //Rcout<<rbeta(1, 1 + matrix_n1(1, 1), 1 + matrix_n0(1, 1))(0)<<"\n";
  //Rcout<<Var<<"\n";
  for(int it = 0; it < iter; it++){
    
    // Update stick-breaking parameters
    gamma.fill(0.0);
    log_alpha.fill(0.0);
    log_beta.fill(0.0);
    
    for(int j = 1; j < K; j++){
      gamma(0) = gamma(0) + n_k(j);
    }
    log_beta(0) = log(rbeta(1, 1 + n_k(0), eta0 + gamma(0))(0));
    log_alpha(0) = log_beta(0);
    
    for(int k = 1; k < K; k++){
      if(k == K - 1){
        log_beta(k) = 0;
      }else{
        for(int j = k + 1; j < K; j++){
          gamma(k) = gamma(k) + n_k(j);
        }
        log_beta(k) = log(rbeta(1, 1 + n_k(k), eta0 + gamma(k))(0));
      }
      log_alpha(k) = log_beta(k);
      l = 0;
      while(l < k){
        log_alpha(k) = log_alpha(k) + log(1 - exp(log_beta(l)));
        l++;
      }
    }
    //Rcout << exp(log_alpha.t()) << "\n";
    
   // if(store){
   //   alpha_mat.row(it) = exp(log_alpha.t());
   // }
    
    // Update z
    
    for(int i = 0; i < n; i++){
      //logprob_temp.fill(0.0);
      for(int k = 0; k < K; k++){
        logprob_temp(k) = log_alpha(k);
        for(int ii = 0; ii < n; ii++){
          if(ii != i){
            if(k < z(ii)){
              logprob_temp(k) = logprob_temp(k) + log_normpdf(W_f(i, ii), mu(k, z(ii)), sqrt(Var(k, z(ii))));
              //Rcout<<logprob_temp.t()<<"\n";
              // find error here
            }
            else{
              logprob_temp(k) = logprob_temp(k) + log_normpdf(W_f(i, ii), mu(z(ii), k), sqrt(Var(z(ii), k)));
              //Rcout<<logprob_temp.t()<<"\n";
            }
          }
        }
      }
      
      
      //logprob_temp = logprob_temp - min(logprob_temp);
      prob_temp = exp(logprob_temp - logsumexp(logprob_temp)) ;
      prob_temp_2 = wrap(prob_temp);
      
      //Rcout<<prob_temp.t()<<"\n";
      // for(int k = 0; k < K; k++){
      //   if(NumericVector::is_na(prob_temp_2(k))){
      //     prob_temp_2(k) = 0.0;
      //   }
      // 
      // }
      //Rcout<<prob_temp_2<<"\n";
      
      z_temp(i) = Rcpp::RcppArmadillo::sample(sampler, 1, true, prob_temp_2)(0);
      if(z(i) != z_temp(i)){
        n_k(z_temp(i)) = n_k(z_temp(i)) + 1;
        n_k(z(i)) = n_k(z(i)) - 1;
        z(i) = z_temp(i);
      }
      // if(store){
      //   z_store.row(count_2) = z.t();
      //   count_2++;
      // }
    }
    
    
    // Update var
    for(int k = 0; k < K; k++){
      for(int kk = k; kk < K; kk++){
        
        if(k == kk){
          matrix_n(k, kk) = n_k(k) * (n_k(kk) - 1)/2;
          W_sum(k, kk) = sum(vectorise(W_f.elem(find(z == k), find(z == kk))))/2;
          W_sum_sq(k, kk) = sum(pow(vectorise(W_f.elem(find(z == k), find(z == kk))), 2))/2;
          if(matrix_n(k, kk) > 0){
            W_sum_sq_2(k, kk) = W_sum_sq(k, kk) - pow(W_sum(k, kk), 2)/matrix_n(k, kk);
            Var(k, kk) = 1/rgamma(1, (matrix_n(k, kk) + nu0)/2,
                2/(SS0 + W_sum_sq_2(k, kk) + ((n0*matrix_n(k, kk))/(n0 + matrix_n(k, kk)))*pow(W_sum(k, kk)/matrix_n(k, kk) - mu0, 2)))(0);
          }else{
            W_sum_sq_2(k, kk) = 0.0;
            Var(k, kk) = SS0;
          }
          
        }
        else{
          matrix_n(k, kk) = n_k(k) * n_k(kk);
          W_sum(k, kk) = sum(vectorise(W_f.elem(find(z == k), find(z == kk))));
          W_sum_sq(k, kk) = sum(pow(vectorise(W_f.elem(find(z == k), find(z == kk))), 2));
          if(matrix_n(k, kk) > 0){
            W_sum_sq_2(k, kk) = W_sum_sq(k, kk) - pow(W_sum(k, kk), 2)/matrix_n(k, kk);
            Var(k, kk) = 1/rgamma(1, (matrix_n(k, kk) + nu0)/2,
                2/(SS0 + W_sum_sq_2(k, kk) + ((n0*matrix_n(k, kk))/(n0 + matrix_n(k, kk)))*pow(W_sum(k, kk)/matrix_n(k, kk) - mu0, 2)))(0);
          }else{
            W_sum_sq_2(k, kk) = 0.0;
            Var(k, kk) = SS0;
          }
        }
      }
    }
    
    
    // Update mu
    
    for(int k = 0; k < K; k++){
      for(int kk = k; kk < K; kk++){
        mu(k, kk) = rnorm(1, (W_sum(k, kk) + n0*mu0)/(matrix_n(k, kk) + n0), sqrt(Var(k, kk)/(matrix_n(k, kk) + n0)))(0);
        if(store){
          logpost_store(it) = logpost_store(it) + (-1.0*matrix_n(k, kk)/2.0) * log(Var(k, kk)) - W_sum_sq(k, kk)/(2.0*Var(k, kk))
          + mu(k, kk)*W_sum(k, kk)/Var(k, kk) - matrix_n(k, kk)*pow(mu(k, kk), 2.0)/(2.0*Var(k, kk));
          logpost_store(it) = logpost_store(it) - 0.5*log(Var(k, kk)/n0) - (n0/(2.0*Var(k, kk))) * pow(mu(k, kk) - mu0, 2.0);
          logpost_store(it) = logpost_store(it) - (nu0/2 + 1) * log(Var(k, kk)) + SS0/(2*Var(k, kk));
        }
      }
    }
    
    
    
    
    //Rcout<<W_sum_sq_2<<"\n";
    
    // if(store){
    //   n_k_store.slice(it) = W_sum;
    // }
    
    // Take the inv-fisher transformation of mu for interpretation
    if(store){
      z_store.row(it) = z.t();
      if(it >= burn){
        //z_store.row(it - burn) = z.t();
        mu_store.slice(it - burn) = mu;
        var_store.slice(it - burn) = Var;
      }
    }
    
    if(it*100/iter == count){
      Rcout<<count<< "% has been done\n";
      //Rcout<<z.t()<<'\n';
      count = count + 10;
    }
  }
  
  
  return Rcpp::List::create(Rcpp::Named("z") = z,
                            Rcpp::Named("z_store") = z_store,
                            //Rcpp::Named("alpha_mat") = alpha_mat,
                            //Rcpp::Named("n_k_store") = n_k_store,
                            Rcpp::Named("mu") = mu,
                            Rcpp::Named("mu_store") = mu_store,
                            Rcpp::Named("Var") = Var,
                            Rcpp::Named("var_store") = var_store,
                            Rcpp::Named("LogL") = LogL,
                            Rcpp::Named("logpost_store") = logpost_store
  );
}

Mat<double> fisher(Mat<double> W){
  return 0.5 * log((1 + W)/(1 - W));
}

double logsumexp(Col<double> x){
  double c = max(x);
  return c + log(sum(exp(x - c)));
}










