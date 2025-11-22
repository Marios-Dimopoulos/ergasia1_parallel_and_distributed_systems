A = Problem.A;
[i,j] = find(A);
writematrix([i,j], 'edges.txt', 'Delimiter',' ');
max(conncomp(graph(Problem.A)))