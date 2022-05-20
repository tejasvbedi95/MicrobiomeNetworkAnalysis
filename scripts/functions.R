# Fisher transformation functions

fisher <- function(r){
  return(0.5 * log((1 + r)/(1 - r)))
}

inv_fisher <- function(r_inv){
  return((exp(2 * r_inv) - 1)/(exp(2 * r_inv) + 1))
}

# Simulating the WSBM weight (correlation) matrix

WSBM_sim <- function(n = 100, K = 4, mu_true, var_true = matrix(0.1, K, K),
                     n_k = c(rep(floor(n/K), K - 1), n - sum(rep(floor(n/K), K - 1))),
                     seed = 1){
  set.seed(seed)
  
  mu_true_f <- fisher(mu_true)
  
  z_true <- NULL
  for (k in 1:K) {
    z_true <- c(z_true, rep(k, n_k[k]))
  }
  W_f <- matrix(0, nrow = n, ncol = n)
  
  for (i in 1:(dim(W_f)[1] - 1)) {
    for (j in (i + 1):dim(W_f)[2]) {
      if (z_true[i] < z_true[j]) {
        W_f[i, j] <- rnorm(1, mean = mu_true_f[z_true[i], z_true[j]], sd = sqrt(var_true[z_true[i], z_true[j]]))
      } else {
        W_f[i, j] <- rnorm(1, mean = mu_true_f[z_true[j], z_true[i]], sd = sqrt(var_true[z_true[j], z_true[i]]))
      }
      W_f[j, i] <- W_f[i, j]
    }
  }
  
  cor.mat <- inv_fisher(W_f)
  return(list(cor.mat = cor.mat, z_true = z_true))
  
}

# Converting taxon abundance count data to correlation matrix for compositional & CLR settings

count_to_cor <- function(data, method = "spearman"){
  
  # Discarding taxa with < 5% +ve counts (filteration step)
  
  data.ind <- apply(data, 2, function(i){
    ifelse(i > 0, 1, 0)
  })
  
  taxa.names <- names(which(apply(data.ind, 2, mean) > 0.05))
  
  data <- data[, taxa.names]
  
  # Compositional Data
  data.comp <- data/rowSums(data)
  cor_comp <- as.matrix(cor(data.comp, method = method))
  diag(cor_comp) <- 0
  
  if(any(cor_comp == 1)){ # Fisher transformation fails if correlation exactly equals 1 or -1
    cor_comp <- cor_comp - 0.000001
  }
  diag(cor_comp) <- 0
  
  # CLR Transformation
  data.CLR <- data/rowSums(data) + 1e-7 # adding pseudocount to avoid 0 for geometric mean
  data.CLR <- data.CLR/rowSums(data.CLR) # rescaling again
  
  gm_fun <- function(x){ # geometric mean function
    return(exp((1/length(x))*sum(log(x))))
  }
  
  CLR_data <- NULL
  for(i in 1:nrow(data.CLR)){
    CLR_data <- rbind(CLR_data, log(data.CLR[i,]/gm_fun(data.CLR[i,])))
  }
  
  cor_CLR <- as.matrix(cor(CLR_data, method = method))
  diag(cor_CLR) <- 0
  
  if(any(cor_CLR == 1)){ # Fisher transformation fails if correlation exactly equals 1 or -1
    cor_CLR <- cor_CLR - 0.000001
  }
  diag(cor_CLR) <- 0
  
  return(list(comp = cor_comp, CLR = cor_CLR))
}


# Clustering Measures

NVI <- function(clust_true, clust_est){ # Normalized Variation of Information
  t <- length(clust_true)
  M1 <- length(unique(clust_true))
  M2 <- length(unique(clust_est)) ## May need different M for rjmcmc
  e1 <- e2 <- mi <- 0
  for(m1 in 1:M1){
    lm1 <- sum(clust_true == m1)
    e1 <- sum(e1, lm1*log(lm1/t))
  }
  for(m2 in 1:M2){
    lm2 <- sum(clust_est == m2)
    e2 <- sum(e2, lm2*log(lm2/t))
  }
  
  for(m1 in 1:M1){
    lm1 <- sum(clust_true == m1)
    for(m2 in 1:M2){
      lm2 <- sum(clust_est == m2)
      cp <- sum((clust_true == m1)*(clust_est == m2))
      if(cp != 0){
        mi <- sum(mi, cp*log((cp*t)/(lm1*lm2)))
      }
    }
  }
  #browser()
  nvi <- -(1/(t*log(t)))*(e1 + e2 + 2*mi)
  return(nvi)
}

MI <- function(clust_true, clust_est){ # Mutual Information
  t <- length(clust_true)
  M1 <- length(unique(clust_true))
  M2 <- length(unique(clust_est)) ## May need different M for rjmcmc
  mi <- 0
  for(m1 in 1:M1){
    lm1 <- sum(clust_true == m1)
    for(m2 in 1:M2){
      lm2 <- sum(clust_est == m2)
      cp <- sum((clust_true == m1)*(clust_est == m2))
      if(cp != 0){
        mi <- sum(mi, cp*log((cp*t)/(lm1*lm2)))
      }
    }
  }
  mi <- mi/t
  return(mi)
}

NMI <- function(clust_true, clust_est){ # Normalized Mutual Information
  t <- length(clust_true)
  M1 <- length(unique(clust_true))
  M2 <- length(unique(clust_est)) ## May need different M for rjmcmc
  e1 <- e2 <- mi <- 0
  
  for(m1 in 1:M1){
    lm1 <- sum(clust_true == m1)
    e1 <- sum(e1, lm1*log(lm1/t))
  }
  for(m2 in 1:M2){
    lm2 <- sum(clust_est == m2)
    e2 <- sum(e2, lm2*log(lm2/t))
  }
  
  for(m1 in 1:M1){
    lm1 <- sum(clust_true == m1)
    for(m2 in 1:M2){
      lm2 <- sum(clust_est == m2)
      cp <- sum((clust_true == m1)*(clust_est == m2))
      if(cp != 0){
        mi <- sum(mi, cp*log((cp*t)/(lm1*lm2)))
      }
    }
  }
  nmi <- mi/(sqrt(e1 * e2))
  return(nmi)
}


ARI <- function(clust_true, clust_est){ # Adjusted rand index
  T1 <- length(clust_true)
  T2 <- length(clust_est) ## May need different M for rjmcmc
  a <- b <- c <- d <- 0
  for(t1 in 2:T1){
    t2 <- 1
    while(t2 < t1){
      a <- sum(a, (clust_true[t1] == clust_true[t2])*(clust_est[t1] == clust_est[t2]))
      b <- sum(b, (clust_true[t1] == clust_true[t2])*(clust_est[t1] != clust_est[t2]))
      c <- sum(c, (clust_true[t1] != clust_true[t2])*(clust_est[t1] == clust_est[t2]))
      d <- sum(d, (clust_true[t1] != clust_true[t2])*(clust_est[t1] != clust_est[t2]))
      t2 <- t2 + 1
    }
  }
  ARI <- (choose(T1, 2)*(a + d) - ((a + b)*(a + c) + (c + d)*(b + d)))/(choose(T1, 2)^2 - ((a + b)*(a + c) + (c + d)*(b + d)))
  return(ARI)
}










