# Microbiome Network Analysis
 
## Tutorial

The following script is used to fit a Weighted Stochastic Infinite Block Model (WSIBM) to analyze real valued weighted adjacency matrices. We consider data in the form of a correlation matrix obtained by performing central log ratio transformation (CLR) on microbiome count data (taxonomic abundance table). WSIBM performs model-based clustering on the correlation matrix in order to detect the latent community structure as well as estimate the distribution of number of communities via Dirichlet Process stick-breaking. We further evaluate the posterior summaries via Posterior Probability Matrix (PPM) which enables us to solve the label switching problem of clustering. 

## Required Packages

```r
# remotes::install_github("GraceYoon/SPRING") (for MAC)
# devtools::install_github("GraceYoon/SPRING") (for Windows)

library(Rcpp)
library(tidyverse)
library(mcclust)
library(MASS)
library(ggpubr)
library(SPRING)
```

## Required Functions

```r
sourceCpp("scripts/SBM_cpp_v3.4.cpp") # CPP function for WSBM (auto)
sourceCpp("scripts/SBM_cpp_v2.5.cpp") # CPP function for WSBM (fixed)
source("scripts/functions.R")
```

## Simulation Study

The following are the data components and model parameters:

-   `n`: Number of nodes (taxa) in the network 
-   `W`: Weighted adjacency matrix (correlation matrix with diagonals set to 0)
-   `K`: Number of communities
-   `z`: Latent community membership vector
-   `mu`: Matrix of weighted block means
-   `var`: Matrix of weighted block variance
-   `K_max`: Upper bound over K
-   `eta0`: Dirichlet process concentration parameter 


``` r
# Simulating the data 
n <- 100
K <- 4
mu_true <- diag(c(-0.9, 0.7, 0.4, -0.5))
mu_true[1, 2] <- 0.3
mu_true[2, 3] <- -0.3
mu_true[1, 3] <- mu_true[1, 4] <- mu_true[2, 4] <- mu_true[3, 4] <- 0.001

cor.sim <- WSBM_sim(n = 100, K = 4, mu_true = mu_true, var_true = matrix(0.1, K, K),
                    seed = 1) # True correlation matrix
cor.mat <- cor.sim$cor.mat

# Visualizing the true correlation matrix

cor.mat.melt <- reshape2::melt(cor.mat)
colnames(cor.mat.melt) <- c("Row", "Col", "Correlation")

g1 <- ggplot(data = cor.mat.melt, aes(x = Row, y = Col)) +
  geom_tile(aes(fill = Correlation), size = 0.25) +
  theme_light() +
  scale_fill_gradient2(low = "pink", mid = "white", high = "green", limits = c(-1, 1)) +
  labs(title = "Real Data")

# Permutating the matrix to mimic real data

sample_id <- sample.int(n = n, size = n, replace = F)
cor.mat.temp <- cor.mat[sample_id, sample_id]
rownames(cor.mat.temp) <- colnames(cor.mat.temp) <- 1:n

cor.mat.melt <- reshape2::melt(cor.mat.temp)
colnames(cor.mat.melt) <- c("Row", "Col", "Correlation")

g2 <- ggplot(data = cor.mat.melt, aes(x = Row, y = Col)) +
  geom_tile(aes(fill = Correlation), size = 0.25) +
  theme_light() +
  scale_fill_gradient2(low = "pink", mid = "white", high = "green", limits = c(-1, 1)) +
  labs(title = "Permutated Data")

ggarrange(g1, g2)
```
![alt text](https://github.com/tejasvbedi95/MicrobiomeNetworkAnalysis/blob/b86e8f316be064c700355a4ff974a93973bb3533/Results/sim_plot_1.png)

``` r
# Fit the permutated data to WSBM function

res <- auto_WSBM(cor.mat.temp, K_max = 20, eta0 = 1, store = T) 

# Obtaining z_ppm (clustering result)

diag(res$ppm_store) <- 5000
clust_res <- minbinder(res$ppm_store/5000, method = "comp")$cl

names(clust_res) <- sample_id
clust_res_arranged <- clust_res[order(as.numeric(names(clust_res)))]

W_data_temp <- cbind(order = clust_res, cor.mat.temp)
W_data_order <- W_data_temp[, -1][order(W_data_temp[, 1]), order(W_data_temp[, 1])]
rownames(W_data_order) <- colnames(W_data_order) <- 1:n

cor.res.melt <- reshape2::melt(W_data_order)
colnames(cor.res.melt) <- c("Row", "Col", "Correlation")

g3 <- ggplot(data = cor.res.melt, aes(x = Row, y = Col)) +
  geom_tile(aes(fill = Correlation), size = 0.25) +
  theme_light() +
  scale_fill_gradient2(low = "pink", mid = "white", high = "green", limits = c(-1, 1)) +
  labs(title = "Clustering Result")

g3 # Plot of clustering result
```
![alt text](https://github.com/tejasvbedi95/MicrobiomeNetworkAnalysis/blob/b86e8f316be064c700355a4ff974a93973bb3533/Results/sim_plot_2.png)

``` r
# External cluster validation
z_true <- cor.sim$z_true

ARI(z_true, clust_res_arranged) # = 1, perfect agreement
NMI(z_true, clust_res_arranged) # = 1, perfect agreement
NVI(z_true, clust_res_arranged) # = 0, perfect agreement

```

## Real Data Analysis
``` r

# Load the synthetic count data
data <- read.csv("metaphlan_qc.csv", header = T)
data <- data[, -c(1:2)]

# Model Fitting via MCLR transformation and SPR correlation with automatic community detection

res <- WSBM_wrapper(data, K = "auto", cor = "SPR", transform = "MCLR",
                         K_max = 20, eta0 = 0.1)

W_data_temp <- cbind(order = res$cluster_labels, res$cor_mat)
W_data_order <- W_data_temp[, -1][order(W_data_temp[, 1]), order(W_data_temp[, 1])]

sort_vec <- cumsum(tapply(sort(res$cluster_labels), as.factor(sort(res$cluster_labels)), length)) + 0.5

cor.res.melt <- reshape2::melt(res$cor_mat)
colnames(cor.res.melt) <- c("Row", "Col", "Correlation")

# Raw data 

g4 <- ggplot(data = cor.res.melt, aes(x = Row, y = Col, fill = Correlation)) +
  geom_tile(color = "white", size = 0.25) +
  theme_light() +
  theme(axis.text.x = element_text(angle = 90, size = 5),
        axis.text.y = element_text(size = 4)) +
  labs(x="", y="") +
  scale_fill_gradient2(low = "pink", mid = "white", high = "green", limits = c(-1, 1)) +
  labs(title = "Real Data")
  
# Clustered data after fitting WSBM

cor.res.melt <- reshape2::melt(W_data_order)
colnames(cor.res.melt) <- c("Row", "Col", "Correlation")

g5 <- ggplot(data = cor.res.melt, aes(x = Row, y = Col, fill = Correlation)) +
  geom_tile(color = "white", size = 0.25) +
  theme_light() +
  theme(axis.text.x = element_text(angle = 90, size = 5),
        axis.text.y = element_text(size = 4)) +
  labs(x="", y="") +
  scale_fill_gradient2(low = "pink", mid = "white", high = "green", limits = c(-1, 1)) +
  geom_hline(yintercept = sort_vec, size = 0.7, linetype = "dashed", col = "red", alpha = 0.4) +
  geom_vline(xintercept = sort_vec, size = 0.7, linetype = "dashed", col = "red", alpha = 0.4) +
  labs(title = "Clustering Result")

ggarrange(g4, g5)


```
![alt text](https://github.com/tejasvbedi95/MicrobiomeNetworkAnalysis/blob/6e7e1805597ef256516af70c0fa26dd517af8565/Results/res_plot_1.png)










