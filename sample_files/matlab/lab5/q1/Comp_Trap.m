function [area] = Comp_Trap(y_vals, h)
    %% calculates a composite trapezoidal quadrature for n data points
    area = h/2 * (y_vals(1) + 2 * sum(y_vals(2:end-1)) + y_vals(end));


end

